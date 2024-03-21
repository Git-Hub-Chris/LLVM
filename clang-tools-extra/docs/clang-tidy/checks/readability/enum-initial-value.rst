.. title:: clang-tidy - readability-enum-initial-value

readability-enum-initial-value
==============================

Detects explicit initialization of a part of enumerators in an enumeration, and
relying on compiler to initialize the others.

It causes readability issues and reduces the maintainability. When adding new
enumerations, the developers need to be careful for potiential enumeration value
conflicts.

In an enumeration, the following three cases are accepted. 
1. none of enumerators are explicit initialized.
2. the first enumerator is explicit initialized.
3. all of enumerators are explicit initialized.

.. code-block:: c++

  // valid, none of enumerators are initialized.
  enum A {
    e0,
    e1,
    e2,
  };

  // valid, the first enumerator is initialized.
  enum A {
    e0 = 0,
    e1,
    e2,
  };

  // valid, all of enumerators are initialized.
  enum A {
    e0 = 0,
    e1 = 1,
    e2 = 2,
  };

  // invalid, e1 is not explicit initialized.
  enum A {
    e0 = 0,
    e1,
    e2 = 2,
  };
