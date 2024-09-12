//===-- Mustache.cpp ------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Mustache.h"
#include "llvm/Support/Error.h"

using namespace llvm;
using namespace llvm::json;
using namespace llvm::mustache;

SmallString<128> escapeString(StringRef Input,
                              DenseMap<char, StringRef> &Escape) {
  SmallString<128> EscapedString("");
  for (char C : Input) {
    if (Escape.find(C) != Escape.end())
      EscapedString += Escape[C];
    else
      EscapedString += C;
  }
  return EscapedString;
}

Accessor split(StringRef Str, char Delimiter) {
  Accessor Tokens;
  if (Str == ".") {
    Tokens.emplace_back(Str);
    return Tokens;
  }
  StringRef Ref(Str);
  while (!Ref.empty()) {
    StringRef Part;
    std::tie(Part, Ref) = Ref.split(Delimiter);
    Tokens.emplace_back(Part.trim());
  }
  return Tokens;
}

Token::Token(StringRef RawBody, StringRef InnerBody, char Identifier)
    : RawBody(RawBody), TokenBody(InnerBody) {
  TokenType = getTokenType(Identifier);
  if (TokenType == Type::Comment)
    return;

  StringRef AccessorStr =
      TokenType == Type::Variable ? InnerBody : InnerBody.substr(1);

  Accessor = split(AccessorStr.trim(), '.');
}

Token::Token(StringRef Str)
    : TokenType(Type::Text), RawBody(Str), Accessor({}), TokenBody(Str) {}

Token::Type Token::getTokenType(char Identifier) {
  switch (Identifier) {
  case '#':
    return Type::SectionOpen;
  case '/':
    return Type::SectionClose;
  case '^':
    return Type::InvertSectionOpen;
  case '!':
    return Type::Comment;
  case '>':
    return Type::Partial;
  case '&':
    return Type::UnescapeVariable;
  default:
    return Type::Variable;
  }
}

// Function to check if there's no meaningful text behind
bool noTextBehind(size_t Idx, const SmallVector<Token, 0> &Tokens) {
  if (Idx == 0 || Tokens[Idx - 1].getType() != Token::Type::Text)
    return false;
  const Token &PrevToken = Tokens[Idx - 1];
  StringRef TokenBody = PrevToken.getRawBody().rtrim(" \t\v\t");
  return TokenBody.ends_with("\n") || TokenBody.ends_with("\r\n") ||
         (TokenBody.empty() && Idx == 1);
}
// Function to check if there's no meaningful text ahead
bool noTextAhead(size_t Idx, const SmallVector<Token, 0> &Tokens) {
  if (Idx >= Tokens.size() - 1 ||
      Tokens[Idx + 1].getType() != Token::Type::Text)
    return false;

  const Token &NextToken = Tokens[Idx + 1];
  StringRef TokenBody = NextToken.getRawBody().ltrim(" ");
  return TokenBody.starts_with("\r\n") || TokenBody.starts_with("\n");
}

// Simple tokenizer that splits the template into tokens
// the mustache spec allows {{{ }}} to unescape variables
// but we don't support that here unescape variable
// is represented only by {{& variable}}
SmallVector<Token, 0> tokenize(StringRef Template) {
  SmallVector<Token, 0> Tokens;
  StringRef Open("{{");
  StringRef Close("}}");
  std::size_t Start = 0;
  std::size_t DelimiterStart = Template.find(Open);
  if (DelimiterStart == StringRef::npos) {
    Tokens.emplace_back(Template);
    return Tokens;
  }
  while (DelimiterStart != StringRef::npos) {
    if (DelimiterStart != Start) {
      Tokens.emplace_back(Template.substr(Start, DelimiterStart - Start));
    }

    size_t DelimiterEnd = Template.find(Close, DelimiterStart);
    if (DelimiterEnd == StringRef::npos) {
      break;
    }

    // Extract the Interpolated variable without {{ and }}
    size_t InterpolatedStart = DelimiterStart + Open.size();
    size_t InterpolatedEnd = DelimiterEnd - DelimiterStart - Close.size();
    SmallString<128> Interpolated =
        Template.substr(InterpolatedStart, InterpolatedEnd);
    SmallString<128> RawBody;
    RawBody += Open;
    RawBody += Interpolated;
    RawBody += Close;

    Tokens.emplace_back(RawBody, Interpolated, Interpolated[0]);
    Start = DelimiterEnd + Close.size();
    DelimiterStart = Template.find(Open, Start);
  }

  if (Start < Template.size())
    Tokens.emplace_back(Template.substr(Start));

  // fix up white spaces for
  // open sections/inverted sections/close section/comment
  // This loop attempts to find standalone tokens and tries to trim out
  // the whitespace around them
  // for example:
  // if you have the template string
  //  "Line 1\n {{#section}} \n Line 2 \n {{/section}} \n Line 3"
  // The output would be
  //  "Line 1\n Line 2\n Line 3"
  size_t LastIdx = Tokens.size() - 1;
  for (size_t Idx = 0, End = Tokens.size(); Idx < End; ++Idx) {
    Token::Type CurrentType = Tokens[Idx].getType();
    // Check if token type requires cleanup
    bool RequiresCleanUp = (CurrentType == Token::Type::SectionOpen ||
                            CurrentType == Token::Type::InvertSectionOpen ||
                            CurrentType == Token::Type::SectionClose ||
                            CurrentType == Token::Type::Comment ||
                            CurrentType == Token::Type::Partial);

    if (!RequiresCleanUp)
      continue;

    bool NoTextBehind = noTextBehind(Idx, Tokens);
    bool NoTextAhead = noTextAhead(Idx, Tokens);

    // Adjust next token body if no text ahead
    if ((NoTextBehind && NoTextAhead) || (NoTextAhead && Idx == 0)) {
      Token &NextToken = Tokens[Idx + 1];
      StringRef NextTokenBody = NextToken.getTokenBody();
      if (NextTokenBody.starts_with("\r\n")) {
        NextToken.setTokenBody(NextTokenBody.substr(2));
      } else if (NextTokenBody.starts_with("\n")) {
        NextToken.setTokenBody(NextTokenBody.substr(1));
      }
    }
    // Adjust previous token body if no text behind
    if ((NoTextBehind && NoTextAhead) || (NoTextBehind && Idx == LastIdx)) {
      Token &PrevToken = Tokens[Idx - 1];
      StringRef PrevTokenBody = PrevToken.getTokenBody();
      PrevToken.setTokenBody(PrevTokenBody.rtrim(" \t\v\t"));
    }
  }
  return Tokens;
}

class Parser {
public:
  Parser(StringRef TemplateStr, BumpPtrAllocator &Allocator)
      : Allocator(Allocator), TemplateStr(TemplateStr) {}

  ASTNode *parse();

private:
  void parseMustache(ASTNode *Parent);

  BumpPtrAllocator &Allocator;
  SmallVector<Token, 0> Tokens;
  size_t CurrentPtr;
  StringRef TemplateStr;
};

ASTNode *Parser::parse() {
  Tokens = tokenize(TemplateStr);
  CurrentPtr = 0;
  void *Root = Allocator.Allocate(sizeof(ASTNode), alignof(ASTNode));
  ASTNode *RootNode = new (Root) ASTNode();
  parseMustache(RootNode);
  return RootNode;
}

void Parser::parseMustache(ASTNode *Parent) {

  while (CurrentPtr < Tokens.size()) {
    Token CurrentToken = Tokens[CurrentPtr];
    CurrentPtr++;
    Accessor A = CurrentToken.getAccessor();
    ASTNode *CurrentNode;
    void *Node = Allocator.Allocate(sizeof(ASTNode), alignof(ASTNode));

    switch (CurrentToken.getType()) {
    case Token::Type::Text: {
      CurrentNode = new (Node) ASTNode(CurrentToken.getTokenBody(), Parent);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::Variable: {
      CurrentNode = new (Node) ASTNode(ASTNode::Variable, A, Parent);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::UnescapeVariable: {
      CurrentNode = new (Node) ASTNode(ASTNode::UnescapeVariable, A, Parent);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::Partial: {
      CurrentNode = new (Node) ASTNode(ASTNode::Partial, A, Parent);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::SectionOpen: {
      CurrentNode = new (Node) ASTNode(ASTNode::Section, A, Parent);
      size_t Start = CurrentPtr;
      parseMustache(CurrentNode);
      size_t End = CurrentPtr;
      SmallString<128> RawBody;
      if (Start + 1 < End - 1) {
        for (std::size_t I = Start + 1; I < End - 1; I++)
          RawBody += Tokens[I].getRawBody();
      } else if (Start + 1 == End - 1) {
        RawBody = Tokens[Start].getRawBody();
      }
      CurrentNode->setRawBody(RawBody);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::InvertSectionOpen: {
      CurrentNode = new (Node) ASTNode(ASTNode::InvertSection, A, Parent);
      size_t Start = CurrentPtr;
      parseMustache(CurrentNode);
      size_t End = CurrentPtr;
      SmallString<128> RawBody;
      if (Start + 1 < End - 1) {
        for (size_t Idx = Start + 1; Idx < End - 1; Idx++)
          RawBody += Tokens[Idx].getRawBody();
      } else if (Start + 1 == End - 1) {
        RawBody = Tokens[Start].getRawBody();
      }
      CurrentNode->setRawBody(RawBody);
      Parent->addChild(CurrentNode);
      break;
    }
    case Token::Type::SectionClose:
      return;
    default:
      break;
    }
  }
}

SmallString<128> Template::render(Value Data) {
  BumpPtrAllocator LocalAllocator;
  return Tree->render(Data, LocalAllocator, Partials, Lambdas, SectionLambdas,
                      Escapes);
}

void Template::registerPartial(StringRef Name, StringRef Partial) {
  Parser P = Parser(Partial, Allocator);
  ASTNode *PartialTree = P.parse();
  Partials[Name] = PartialTree;
}

void Template::registerLambda(StringRef Name, Lambda L) { Lambdas[Name] = L; }

void Template::registerLambda(StringRef Name, SectionLambda L) {
  SectionLambdas[Name] = L;
}

void Template::registerEscape(DenseMap<char, StringRef> E) { Escapes = E; }

Template::Template(StringRef TemplateStr) {
  Parser P = Parser(TemplateStr, Allocator);
  Tree = P.parse();
  // the default behaviour is to escape html entities
  DenseMap<char, StringRef> HtmlEntities = {{'&', "&amp;"},
                                            {'<', "&lt;"},
                                            {'>', "&gt;"},
                                            {'"', "&quot;"},
                                            {'\'', "&#39;"}};
  registerEscape(HtmlEntities);
}

SmallString<128> printJson(Value &Data) {

  SmallString<128> Result;
  if (Data.getAsNull())
    return Result;
  if (auto *Arr = Data.getAsArray())
    if (Arr->empty())
      return Result;
  if (Data.getAsString()) {
    Result += Data.getAsString()->str();
    return Result;
  }
  return formatv("{0:2}", Data);
}

bool isFalsey(Value &V) {
  return V.getAsNull() || (V.getAsBoolean() && !V.getAsBoolean().value()) ||
         (V.getAsArray() && V.getAsArray()->empty()) ||
         (V.getAsObject() && V.getAsObject()->empty());
}

SmallString<128>
ASTNode::render(Value Data, BumpPtrAllocator &Allocator,
                DenseMap<StringRef, ASTNode *> &Partials,
                DenseMap<StringRef, Lambda> &Lambdas,
                DenseMap<StringRef, SectionLambda> &SectionLambdas,
                DenseMap<char, StringRef> &Escapes) {
  LocalContext = Data;
  Value Context = T == Root ? Data : findContext();
  SmallString<128> Result;
  switch (T) {
  case Root: {
    for (ASTNode *Child : Children)
      Result += Child->render(Context, Allocator, Partials, Lambdas,
                              SectionLambdas, Escapes);
    return Result;
  }
  case Text:
    return Body;
  case Partial: {
    if (Partials.find(Accessor[0]) != Partials.end()) {
      ASTNode *Partial = Partials[Accessor[0]];
      Result += Partial->render(Data, Allocator, Partials, Lambdas,
                                SectionLambdas, Escapes);
      return Result;
    }
    return Result;
  }
  case Variable: {
    if (Lambdas.find(Accessor[0]) != Lambdas.end()) {
      Lambda &L = Lambdas[Accessor[0]];
      Value LambdaResult = L();
      StringRef LambdaStr = printJson(LambdaResult);
      Parser P = Parser(LambdaStr, Allocator);
      ASTNode *LambdaNode = P.parse();
      SmallString<128> RenderStr = LambdaNode->render(
          Data, Allocator, Partials, Lambdas, SectionLambdas, Escapes);
      return escapeString(RenderStr, Escapes);
    }
    return escapeString(printJson(Context), Escapes);
  }
  case UnescapeVariable: {
    if (Lambdas.find(Accessor[0]) != Lambdas.end()) {
      Lambda &L = Lambdas[Accessor[0]];
      Value LambdaResult = L();
      StringRef LambdaStr = printJson(LambdaResult);
      Parser P = Parser(LambdaStr, Allocator);
      ASTNode *LambdaNode = P.parse();
      DenseMap<char, StringRef> EmptyEscapes;
      return LambdaNode->render(Data, Allocator, Partials, Lambdas,
                                SectionLambdas, EmptyEscapes);
    }
    return printJson(Context);
  }
  case Section: {
    // Sections are not rendered if the context is falsey
    bool IsLambda = SectionLambdas.find(Accessor[0]) != SectionLambdas.end();
    if (isFalsey(Context) && !IsLambda)
      return Result;
    if (IsLambda) {
      SectionLambda &Lambda = SectionLambdas[Accessor[0]];
      Value Return = Lambda(RawBody);
      if (isFalsey(Return))
        return Result;
      StringRef LambdaStr = printJson(Return);
      Parser P = Parser(LambdaStr.str(), Allocator);
      ASTNode *LambdaNode = P.parse();
      return LambdaNode->render(Data, Allocator, Partials, Lambdas,
                                SectionLambdas, Escapes);
    }
    if (Context.getAsArray()) {
      json::Array *Arr = Context.getAsArray();
      for (Value &V : *Arr) {
        for (ASTNode *Child : Children)
          Result += Child->render(V, Allocator, Partials, Lambdas,
                                  SectionLambdas, Escapes);
      }
      return Result;
    }
    for (ASTNode *Child : Children)
      Result += Child->render(Context, Allocator, Partials, Lambdas,
                              SectionLambdas, Escapes);
    return Result;
  }
  case InvertSection: {
    bool IsLambda = SectionLambdas.find(Accessor[0]) != SectionLambdas.end();
    if (!isFalsey(Context) || IsLambda)
      return Result;
    for (ASTNode *Child : Children)
      Result += Child->render(Context, Allocator, Partials, Lambdas,
                              SectionLambdas, Escapes);
    return Result;
  }
  }
  llvm_unreachable("Invalid ASTNode type");
}

Value ASTNode::findContext() {
  // The mustache spec allows for dot notation to access nested values
  // a single dot refers to the current context
  // We attempt to find the JSON context in the current node if it is not found
  // we traverse the parent nodes to find the context until we reach the root
  // node or the context is found
  if (Accessor.empty())
    return nullptr;
  if (Accessor[0] == ".")
    return LocalContext;
  json::Object *CurrentContext = LocalContext.getAsObject();
  StringRef CurrentAccessor = Accessor[0];
  ASTNode *CurrentParent = Parent;

  while (!CurrentContext || !CurrentContext->get(CurrentAccessor)) {
    if (CurrentParent->T != Root) {
      CurrentContext = CurrentParent->LocalContext.getAsObject();
      CurrentParent = CurrentParent->Parent;
      continue;
    }
    return nullptr;
  }
  Value Context = nullptr;
  for (auto E : enumerate(Accessor)) {
    Value *CurrentValue = CurrentContext->get(E.value());
    if (!CurrentValue)
      return nullptr;
    if (E.index() < Accessor.size() - 1) {
      CurrentContext = CurrentValue->getAsObject();
      if (!CurrentContext)
        return nullptr;
    } else
      Context = *CurrentValue;
  }
  return Context;
}
