// RUN: rm -rf %t && mkdir -p %t
// RUN: clang-doc --format=html --output=%t --executor=standalone %s
// RUN: clang-doc --format=md --output=%t --executor=standalone %s
// RUN: FileCheck %s < %t/index_json.js -check-prefix=JSON-INDEX
// RUN: FileCheck %s < %t/@nonymous_namespace/AnonClass.html -check-prefix=HTML-ANON-CLASS
// RUN: FileCheck %s < %t/@nonymous_namespace/index.html -check-prefix=HTML-ANON-INDEX
// RUN: FileCheck %s < %t/AnotherNamespace/ClassInAnotherNamespace.html -check-prefix=HTML-ANOTHER-CLASS
// RUN: FileCheck %s < %t/AnotherNamespace/index.html -check-prefix=HTML-ANOTHER-INDEX
// RUN: FileCheck %s < %t/GlobalNamespace/index.html -check-prefix=HTML-GLOBAL-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/NestedNamespace/ClassInNestedNamespace.html -check-prefix=HTML-NESTED-CLASS
// RUN: FileCheck %s < %t/PrimaryNamespace/NestedNamespace/index.html -check-prefix=HTML-NESTED-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/index.html -check-prefix=HTML-PRIMARY-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/ClassInPrimaryNamespace.html -check-prefix=HTML-PRIMARY-CLASS
// RUN: FileCheck %s < %t/@nonymous_namespace/AnonClass.md -check-prefix=MD-ANON-CLASS
// RUN: FileCheck %s < %t/@nonymous_namespace/index.md -check-prefix=MD-ANON-INDEX
// RUN: FileCheck %s < %t/AnotherNamespace/ClassInAnotherNamespace.md -check-prefix=MD-ANOTHER-CLASS
// RUN: FileCheck %s < %t/AnotherNamespace/index.md -check-prefix=MD-ANOTHER-INDEX
// RUN: FileCheck %s < %t/GlobalNamespace/index.md -check-prefix=MD-GLOBAL-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/NestedNamespace/ClassInNestedNamespace.md -check-prefix=MD-NESTED-CLASS
// RUN: FileCheck %s < %t/PrimaryNamespace/NestedNamespace/index.md -check-prefix=MD-NESTED-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/index.md -check-prefix=MD-PRIMARY-INDEX
// RUN: FileCheck %s < %t/PrimaryNamespace/ClassInPrimaryNamespace.md -check-prefix=MD-PRIMARY-CLASS
// RUN: FileCheck %s < %t/all_files.md -check-prefix=MD-ALL-FILES
// RUN: FileCheck %s < %t/index.md -check-prefix=MD-INDEX

// Anonymous Namespace
namespace
{
    void anonFunction() {}
    class AnonClass {};
}

// Primary Namespace
namespace PrimaryNamespace {
    // Function in PrimaryNamespace
    void functionInPrimaryNamespace() {}

    // Class in PrimaryNamespace
    class ClassInPrimaryNamespace {};

    // Nested namespace
    namespace NestedNamespace {
        // Function in NestedNamespace
        void functionInNestedNamespace() {}
        // Class in NestedNamespace
        class ClassInNestedNamespace {};
    }
}

// AnotherNamespace
namespace AnotherNamespace {
    // Function in AnotherNamespace
    void functionInAnotherNamespace() {}
    // Class in AnotherNamespace
    class ClassInAnotherNamespace {};
}

// JSON-INDEX: async function LoadIndex() {
// JSON-INDEX-NEXT: return{
// JSON-INDEX-NEXT:   "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:   "Name": "",
// JSON-INDEX-NEXT:   "RefType": "default",
// JSON-INDEX-NEXT:   "Path": "",
// JSON-INDEX-NEXT:   "Children": [
// JSON-INDEX-NEXT:     {
// JSON-INDEX-NEXT:       "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:       "Name": "@nonymous_namespace",
// JSON-INDEX-NEXT:       "RefType": "namespace",
// JSON-INDEX-NEXT:       "Path": "@nonymous_namespace",
// JSON-INDEX-NEXT:       "Children": [
// JSON-INDEX-NEXT:         {
// JSON-INDEX-NEXT:           "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:           "Name": "AnonClass",
// JSON-INDEX-NEXT:           "RefType": "record",
// JSON-INDEX-NEXT:           "Path": "@nonymous_namespace",
// JSON-INDEX-NEXT:           "Children": []
// JSON-INDEX-NEXT:         }
// JSON-INDEX-NEXT:       ]
// JSON-INDEX-NEXT:     },
// JSON-INDEX-NEXT:     {
// JSON-INDEX-NEXT:       "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:       "Name": "AnotherNamespace",
// JSON-INDEX-NEXT:       "RefType": "namespace",
// JSON-INDEX-NEXT:       "Path": "AnotherNamespace",
// JSON-INDEX-NEXT:       "Children": [
// JSON-INDEX-NEXT:         {
// JSON-INDEX-NEXT:           "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:           "Name": "ClassInAnotherNamespace",
// JSON-INDEX-NEXT:           "RefType": "record",
// JSON-INDEX-NEXT:           "Path": "AnotherNamespace",
// JSON-INDEX-NEXT:           "Children": []
// JSON-INDEX-NEXT:         }
// JSON-INDEX-NEXT:       ]
// JSON-INDEX-NEXT:     },
// JSON-INDEX-NEXT:     {
// JSON-INDEX-NEXT:       "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:       "Name": "GlobalNamespace",
// JSON-INDEX-NEXT:       "RefType": "namespace",
// JSON-INDEX-NEXT:       "Path": "GlobalNamespace",
// JSON-INDEX-NEXT:       "Children": []
// JSON-INDEX-NEXT:     },
// JSON-INDEX-NEXT:     {
// JSON-INDEX-NEXT:       "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:       "Name": "PrimaryNamespace",
// JSON-INDEX-NEXT:       "RefType": "namespace",
// JSON-INDEX-NEXT:       "Path": "PrimaryNamespace",
// JSON-INDEX-NEXT:       "Children": [
// JSON-INDEX-NEXT:         {
// JSON-INDEX-NEXT:           "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:           "Name": "ClassInPrimaryNamespace",
// JSON-INDEX-NEXT:           "RefType": "record",
// JSON-INDEX-NEXT:           "Path": "PrimaryNamespace",
// JSON-INDEX-NEXT:           "Children": []
// JSON-INDEX-NEXT:         },
// JSON-INDEX-NEXT:         {
// JSON-INDEX-NEXT:           "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:           "Name": "NestedNamespace",
// JSON-INDEX-NEXT:           "RefType": "namespace",
// JSON-INDEX-NEXT:           "Path": "PrimaryNamespace{{[\/]+}}NestedNamespace",
// JSON-INDEX-NEXT:           "Children": [
// JSON-INDEX-NEXT:             {
// JSON-INDEX-NEXT:               "USR": "{{([0-9A-F]{40})}}",
// JSON-INDEX-NEXT:               "Name": "ClassInNestedNamespace",
// JSON-INDEX-NEXT:               "RefType": "record",
// JSON-INDEX-NEXT:               "Path": "PrimaryNamespace{{[\/]+}}NestedNamespace",
// JSON-INDEX-NEXT:               "Children": []
// JSON-INDEX-NEXT:             }
// JSON-INDEX-NEXT:           ]
// JSON-INDEX-NEXT:         }
// JSON-INDEX-NEXT:       ]
// JSON-INDEX-NEXT:     }
// JSON-INDEX-NEXT:   ]
// JSON-INDEX-NEXT: };
// JSON-INDEX-NEXT: }

// HTML-ANON-CLASS: <h1>class AnonClass</h1>
// HTML-ANON-CLASS: <p>Defined at line 30 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>

// HTML-ANON-INDEX: <h1>namespace @nonymous_namespace</h1>
// HTML-ANON-INDEX: <p> Anonymous Namespace</p>
// HTML-ANON-INDEX: <h2 id="Records">Records</h2>
// HTML-ANON-INDEX: <a href="AnonClass.html">AnonClass</a>
// HTML-ANON-INDEX: <h2 id="Functions">Functions</h2>
// HTML-ANON-INDEX: <h3 id="{{([0-9A-F]{40})}}">anonFunction</h3>
// HTML-ANON-INDEX: <p>void anonFunction()</p>
// HTML-ANON-INDEX: <p>Defined at line 29 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>

// HTML-ANOTHER-CLASS: <h1>class ClassInAnotherNamespace</h1>
// HTML-ANOTHER-CLASS: <p>Defined at line 55 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-ANOTHER-CLASS: <p> Class in AnotherNamespace</p>

// HTML-ANOTHER-INDEX: <h1>namespace AnotherNamespace</h1>
// HTML-ANOTHER-INDEX: <p> AnotherNamespace</p>
// HTML-ANOTHER-INDEX: <h2 id="Records">Records</h2>
// HTML-ANOTHER-INDEX: <a href="ClassInAnotherNamespace.html">ClassInAnotherNamespace</a>
// HTML-ANOTHER-INDEX: <h2 id="Functions">Functions</h2>
// HTML-ANOTHER-INDEX: <h3 id="{{([0-9A-F]{40})}}">functionInAnotherNamespace</h3>
// HTML-ANOTHER-INDEX: <p>void functionInAnotherNamespace()</p>
// HTML-ANOTHER-INDEX: <p>Defined at line 53 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-ANOTHER-INDEX: <p> Function in AnotherNamespace</p>

// HTML-GLOBAL-INDEX: <div id="main-content" class="col-xs-12 col-sm-9 col-md-8 main-content">
// HTML-GLOBAL-INDEX: <h1>Global Namespace</h1>
// HTML-GLOBAL-INDEX: <h2 id="Namespaces">Namespaces</h2>
// HTML-GLOBAL-INDEX: <li>@nonymous_namespace</li>
// HTML-GLOBAL-INDEX: <li>PrimaryNamespace</li>
// HTML-GLOBAL-INDEX: <li>AnotherNamespace</li>

// HTML-NESTED-CLASS: <h1>class ClassInNestedNamespace</h1>
// HTML-NESTED-CLASS: <p>Defined at line 46 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-NESTED-CLASS: <p> Class in NestedNamespace</p>

// HTML-NESTED-INDEX: <h1>namespace NestedNamespace</h1>
// HTML-NESTED-INDEX: <p> Nested namespace</p>
// HTML-NESTED-INDEX: <h2 id="Records">Records</h2>
// HTML-NESTED-INDEX: <a href="ClassInNestedNamespace.html">ClassInNestedNamespace</a>
// HTML-NESTED-INDEX: <h2 id="Functions">Functions</h2>
// HTML-NESTED-INDEX: <h3 id="{{([0-9A-F]{40})}}">functionInNestedNamespace</h3>
// HTML-NESTED-INDEX: <p>void functionInNestedNamespace()</p>
// HTML-NESTED-INDEX: <p>Defined at line 44 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-NESTED-INDEX: <p> Function in NestedNamespace</p>

// HTML-PRIMARY-CLASS: <h1>class ClassInPrimaryNamespace</h1>
// HTML-PRIMARY-CLASS: <p>Defined at line 39 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-PRIMARY-CLASS: <p> Class in PrimaryNamespace</p>

// HTML-PRIMARY-INDEX: <h1>namespace PrimaryNamespace</h1>
// HTML-PRIMARY-INDEX: <p> Primary Namespace</p>
// HTML-PRIMARY-INDEX: <h2 id="Namespaces">Namespaces</h2>
// HTML-PRIMARY-INDEX: <a href="NestedNamespace{{[\/]}}index.html">NestedNamespace</a>
// HTML-PRIMARY-INDEX: <h2 id="Records">Records</h2>
// HTML-PRIMARY-INDEX: <a href="ClassInPrimaryNamespace.html">ClassInPrimaryNamespace</a>
// HTML-PRIMARY-INDEX: <h2 id="Functions">Functions</h2>
// HTML-PRIMARY-INDEX: <h3 id="{{([0-9A-F]{40})}}">functionInPrimaryNamespace</h3>
// HTML-PRIMARY-INDEX: <p>void functionInPrimaryNamespace()</p>
// HTML-PRIMARY-INDEX: <p>Defined at line 36 of file {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp</p>
// HTML-PRIMARY-INDEX: <p> Function in PrimaryNamespace</p>

// MD-ANON-CLASS: # class AnonClass
// MD-ANON-CLASS: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#30*

// MD-ANON-INDEX: # namespace @nonymous_namespace
// MD-ANON-INDEX:  Anonymous Namespace
// MD-ANON-INDEX: ## Records
// MD-ANON-INDEX: * [AnonClass](AnonClass.md)
// MD-ANON-INDEX: ## Functions
// MD-ANON-INDEX: ### anonFunction
// MD-ANON-INDEX: *void anonFunction()*
// MD-ANON-INDEX: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#29*

// MD-ANOTHER-CLASS: # class ClassInAnotherNamespace
// MD-ANOTHER-CLASS: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#55*
// MD-ANOTHER-CLASS:  Class in AnotherNamespace

// MD-ANOTHER-INDEX: # namespace AnotherNamespace
// MD-ANOTHER-INDEX: AnotherNamespace
// MD-ANOTHER-INDEX: ## Records
// MD-ANOTHER-INDEX: * [ClassInAnotherNamespace](ClassInAnotherNamespace.md)
// MD-ANOTHER-INDEX: ## Functions
// MD-ANOTHER-INDEX: ### functionInAnotherNamespace
// MD-ANOTHER-INDEX: *void functionInAnotherNamespace()*
// MD-ANOTHER-INDEX: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#53*
// MD-ANOTHER-INDEX: Function in AnotherNamespace

// MD-GLOBAL-INDEX: # Global Namespace
// MD-GLOBAL-INDEX: ## Namespaces
// MD-GLOBAL-INDEX: * [@nonymous_namespace](..{{[\/]}}@nonymous_namespace{{[\/]}}index.md)
// MD-GLOBAL-INDEX: * [PrimaryNamespace](..{{[\/]}}PrimaryNamespace{{[\/]}}index.md)
// MD-GLOBAL-INDEX: * [AnotherNamespace](..{{[\/]}}AnotherNamespace{{[\/]}}index.md)

// MD-NESTED-CLASS: # class ClassInNestedNamespace
// MD-NESTED-CLASS: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#46*
// MD-NESTED-CLASS: Class in NestedNamespace

// MD-NESTED-INDEX: # namespace NestedNamespace
// MD-NESTED-INDEX: Nested namespace
// MD-NESTED-INDEX: ## Records
// MD-NESTED-INDEX: * [ClassInNestedNamespace](ClassInNestedNamespace.md)
// MD-NESTED-INDEX: ## Functions
// MD-NESTED-INDEX: ### functionInNestedNamespace
// MD-NESTED-INDEX: *void functionInNestedNamespace()*
// MD-NESTED-INDEX: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#44*
// MD-NESTED-INDEX: Function in NestedNamespace

// MD-PRIMARY-CLASS: # class ClassInPrimaryNamespace
// MD-PRIMARY-CLASS: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#39*
// MD-PRIMARY-CLASS: Class in PrimaryNamespace

// MD-PRIMARY-INDEX: # namespace PrimaryNamespace
// MD-PRIMARY-INDEX:  Primary Namespace
// MD-PRIMARY-INDEX: ## Namespaces
// MD-PRIMARY-INDEX: * [NestedNamespace](NestedNamespace{{[\/]}}index.md)
// MD-PRIMARY-INDEX: ## Records
// MD-PRIMARY-INDEX: * [ClassInPrimaryNamespace](ClassInPrimaryNamespace.md)
// MD-PRIMARY-INDEX: ## Functions
// MD-PRIMARY-INDEX: ### functionInPrimaryNamespace
// MD-PRIMARY-INDEX: *void functionInPrimaryNamespace()*
// MD-PRIMARY-INDEX: *Defined at {{.*}}clang-tools-extra{{[\/]}}test{{[\/]}}clang-doc{{[\/]}}namespace.cpp#36*
// MD-PRIMARY-INDEX:  Function in PrimaryNamespace

// MD-ALL-FILES: # All Files
// MD-ALL-FILES: ## [@nonymous_namespace](@nonymous_namespace{{[\/]}}index.md)
// MD-ALL-FILES: ## [AnotherNamespace](AnotherNamespace{{[\/]}}index.md)
// MD-ALL-FILES: ## [GlobalNamespace](GlobalNamespace{{[\/]}}index.md)
// MD-ALL-FILES: ## [PrimaryNamespace](PrimaryNamespace{{[\/]}}index.md)

// MD-INDEX: #  C/C++ Reference
// MD-INDEX: * Namespace: [@nonymous_namespace](@nonymous_namespace)
// MD-INDEX: * Namespace: [AnotherNamespace](AnotherNamespace)
// MD-INDEX: * Namespace: [PrimaryNamespace](PrimaryNamespace)