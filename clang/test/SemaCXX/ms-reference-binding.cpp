// RUN: %clang_cc1 -fsyntax-only -Wno-microsoft-reference-binding -verify -fms-reference-binding %s
// RUN: %clang_cc1 -DEXTWARN -fsyntax-only -verify -fms-reference-binding %s

#ifdef EXTWARN

struct A {};
void fARef(A&) {}

void test1() {
  A& a1 = A(); // expected-warning{{binding a user-defined type temporary to a non-const lvalue is a Microsoft extension}}

  fARef(A()); // expected-warning{{binding a user-defined type temporary to a non-const lvalue is a Microsoft extension}}
}

void fARefDoNotWarn(A&) {}
void fARefDoNotWarn(const A&) {}

// expected-note@+2 {{candidate function not viable: 1st argument ('const A') would lose const qualifier}}
// expected-note@+1 {{candidate function not viable: 1st argument ('const A') would lose const qualifier}}
void fARefLoseConstQualifier(A&) {}

void test2() {
  // This should not warn since `fARefDoNotWarn(const A&)` is a better candidate
  fARefDoNotWarn(A());

  const A a;
  fARefLoseConstQualifier(a); // expected-error{{no matching function for call to 'fARefLoseConstQualifier'}}
  fARefLoseConstQualifier(static_cast<const A&&>(a)); // expected-error{{no matching function for call to 'fARefLoseConstQualifier'}}
}

#else

struct A {};
struct B : A {};

typedef A AAlias;

void fADefaultArgRef(A& = A{});
void fBDefaultArgRef(A& = B{});

void fAAliasDefaultArgRef(AAlias& = AAlias{});

B fB();
A fA();

A&& fARvalueRef();
A(&&fARvalueRefArray())[1];

void fADefaultArgRef2(A& = fARvalueRef());

void fARef(A&) {}
void fAAliasRef(AAlias&) {}

// expected-note@+2 {{candidate function not viable: expects an lvalue for 1st argument}}
// expected-note@+1 {{candidate function not viable: expects an lvalue for 1st argument}}
void fAVolatileRef(volatile A&) {}

void fIntRef(int&) {} // expected-note{{candidate function not viable: expects an lvalue for 1st argument}}
void fDoubleRef(double&) {} // expected-note{{candidate function not viable: expects an lvalue for 1st argument}}

void fIntConstRef(const int&) {}
void fDoubleConstRef(const double&) {}

void fIntArray(int (&)[1]); // expected-note{{candidate function not viable: expects an lvalue for 1st argument}}
void fIntConstArray(const int (&)[1]);

namespace NS {
  void fARef(A&) {}
  void fAAliasRef(AAlias&) {}

  // expected-note@+2 {{passing argument to parameter here}}
  // expected-note@+1 {{passing argument to parameter here}}
  void fAVolatileRef(volatile A&) {}

  void fIntRef(int&) {} // expected-note{{passing argument to parameter here}}
  void fDoubleRef(double&) {} // expected-note{{passing argument to parameter here}}

  void fIntConstRef(const int&) {}
  void fDoubleConstRef(const double&) {}

  A(&&fARvalueRefArray())[1];

  void fIntArray(int (&)[1]); // expected-note{{passing argument to parameter here}}

  void fIntConstArray(const int (&)[1]);
}

void test1() {
  double& rd2 = 2.0; // expected-error{{non-const lvalue reference to type 'double' cannot bind to a temporary of type 'double'}}
  int& i1 = 0; // expected-error{{non-const lvalue reference to type 'int' cannot bind to a temporary of type 'int'}}

  fIntRef(0); // expected-error{{no matching function for call to 'fIntRef'}}
  fDoubleRef(0.0); // expected-error{{no matching function for call to 'fDoubleRef'}}

  NS::fIntRef(0); // expected-error{{non-const lvalue reference to type 'int' cannot bind to a temporary of type 'int'}}
  NS::fDoubleRef(0.0); // expected-error{{non-const lvalue reference to type 'double' cannot bind to a temporary of type 'double'}}

  int i2 = 2;
  double& rd3 = i2; // expected-error{{non-const lvalue reference to type 'double' cannot bind to a value of unrelated type 'int'}}
}

void test2() {
  fIntConstRef(0);
  fDoubleConstRef(0.0);

  NS::fIntConstRef(0);
  NS::fDoubleConstRef(0.0);

  int i = 0;
  const int ci = 0;
  volatile int vi = 0;
  const volatile int cvi = 0;
  bool b = true;

  const volatile int &cvir1 = b ? ci : vi; // expected-error{{volatile lvalue reference to type 'const volatile int' cannot bind to a temporary of type 'int'}}

  volatile int& vir1 = 0; // expected-error{{volatile lvalue reference to type 'volatile int' cannot bind to a temporary of type 'int'}}
  const volatile int& cvir2 = 0; // expected-error{{volatile lvalue reference to type 'const volatile int' cannot bind to a temporary of type 'int'}}
}

void test3() {
  A& a1 = A();
  AAlias& aalias1 = A();

  fARef(A());
  fARef(static_cast<A&&>(a1));

  fAAliasRef(A());
  fAAliasRef(static_cast<A&&>(a1));
  fAAliasRef(AAlias());
  fAAliasRef(static_cast<AAlias&&>(a1));

  fAVolatileRef(A()); // expected-error{{no matching function for call to 'fAVolatileRef'}}
  fAVolatileRef(static_cast<A&&>(a1)); // expected-error{{no matching function for call to 'fAVolatileRef'}}

  fARef(B());
  fAAliasRef(B());

  NS::fARef(A());
  NS::fARef(static_cast<A&&>(a1));

  NS::fAAliasRef(A());
  NS::fAAliasRef(static_cast<A&&>(a1));
  NS::fAAliasRef(AAlias());
  NS::fAAliasRef(static_cast<AAlias&&>(a1));

  NS::fAVolatileRef(A()); // expected-error{{volatile lvalue reference to type 'volatile A' cannot bind to a temporary of type 'A'}}
  NS::fAVolatileRef(static_cast<A&&>(a1)); // expected-error{{volatile lvalue reference to type 'volatile A' cannot bind to a temporary of type 'A'}}

  NS::fARef(B());
  NS::fAAliasRef(B());

  A& a2 = fA();
  AAlias& aalias2 = fA();

  A& a3 = fARvalueRef();
  AAlias& aalias3 = fARvalueRef();

  const A& rca = fB();
  A& ra = fB();
  AAlias& raalias = fB();
}

void test4() {
  A (&array1)[1] = fARvalueRefArray(); // expected-error{{non-const lvalue reference to type 'A[1]' cannot bind to a temporary of type 'A[1]'}}
  const A (&array2)[1] = fARvalueRefArray();

  A (&array3)[1] = NS::fARvalueRefArray(); // expected-error{{non-const lvalue reference to type 'A[1]' cannot bind to a temporary of type 'A[1]'}}
  const A (&array4)[1] = NS::fARvalueRefArray();

  fIntArray({ 1 }); // expected-error{{no matching function for call to 'fIntArray'}}
  NS::fIntArray({ 1 }); // expected-error{{non-const lvalue reference to type 'int[1]' cannot bind to an initializer list temporary}}

  fIntConstArray({ 1 });
  NS::fIntConstArray({ 1 });
}

void test5() {
  fADefaultArgRef();
  fADefaultArgRef(A());

  fBDefaultArgRef();
  fBDefaultArgRef(B());
  fBDefaultArgRef(A());

  fAAliasDefaultArgRef(A());
  fAAliasDefaultArgRef(B());
  fAAliasDefaultArgRef(AAlias());
}

struct C { operator A() { return A(); } };
struct D { D(int) {} };

// expected-note@+1 {{candidate function not viable: no known conversion from 'C' to 'A &' for 1st argument}}
void fARefConvOperator(A&);

// expected-note@+1 {{candidate function not viable: no known conversion from 'int' to 'D &' for 1st argument}}
void fDRefTemp(D&);

void fAConstRefConvOperator(const A&);
void fDConstRefTemp(const D&);

void test6() {
  fARefConvOperator(C()); // expected-error{{no matching function for call to 'fARefConvOperator'}}
  fDRefTemp(1); // expected-error{{no matching function for call to 'fDRefTemp'}}

  fAConstRefConvOperator(C());
  fDConstRefTemp(1);

  const A& cARef = C();
  A& ARef = C(); // expected-error{{non-const lvalue reference to type 'A' cannot bind to a temporary of type 'C'}}

  const D& cDRef = 1;
  D& DRef = 1; // expected-error{{non-const lvalue reference to type 'D' cannot bind to a temporary of type 'int'}}
}

A& retARef();
struct E { operator A&() { return retARef(); } };

void test7() {
  const A& cARef = E();
  A& ARef = E();
}

#endif
