## Style guidelines

We follow the guidelines proposed in the small book, [_Embedded C Coding
Standard_](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf) by Michael
Barr, with some exceptions. This standard was chosen because it is available for free and because
it was similar in some superficial ways to the style that we already prefer.

### Project-specific exceptions to the standard
We use some arbitrary exceptions to the Barr standard. These serve
* to minimize the version history noise required to come into compliance, and
* to avoid unnecessary differences between our C style guidelines and our Java style guidelines.

#### Braces (1.3)

We use K&R style braces. Example:
```
if (NULL == p_event) {
    lf_print_error_and_exit("Out of memory!");
}
```

#### The pointer operator * in declarations (3.1g)

We only use a space on the variable name side (RHS) of *. Example:
```
int* ptr;
```

#### Function declarations (3.1j)

We do not use the special case mentioned in Barr 3.1j.

#### Blank lines separating natural blocks of code (3.3b)

The only natural blocks of code that must be preceded and followed by a blank line are procedure
definitions. Elsewhere, blank lines should only be used where they aid readability.

#### End-of-file comments (3.3c)

We do not print out code for the purpose of code review. We do not require end-of-file comments.

#### Header files (4.2a)

Multiple source files corresponding to one header file are permitted if and only if they represent
alternative functionality that depends on a preprocessor definition.

#### Names of public data types (5.1c)

The name of all public data types shall be prefixed with lf_.

#### Names of public functions (6.1i)

The name of all public functions shall be prefixed with lf_.

#### Printed pages (6.2 and elsewhere)

We do not print code for the purpose of code review. In cases where Barr refers to the size of a
printed page, consider the size of a typical computer monitor instead. In particular, all reasonable
effort shall be taken to keep the length of each function limited to a maximum of **40 lines**.

#### Short variable names (7.1e)

We place no restrictions on the length of variable names; however, recommendations against cryptic
abbreviations and abbreviations not in a version-controlled table nor in Barr Appendix A still apply.

### Addenda

#### Inlining-related performance optimizations

Before attempting to encourage or force inlining, e.g. using the `inline` functions and/or
parameterized macros, consider whether similar performance could be realized using link-time
optimization instead.

#### Return types in function declarations

The return type of a function must appear on the same line as the function name.
