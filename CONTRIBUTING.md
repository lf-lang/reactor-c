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

We use K&R-style braces. Example:
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

When defining a function, we do not add a space between the function name and the opening
parenthesis.

#### Horizontal alignment (3.2a, c)
Tabs are always forbidden. The use of multiple consecutive spaces is forbidden except as indentation
at the beginning of a line or as required by rule 3.2d.

#### Blank lines separating natural blocks of code (3.3b)

The only natural blocks of code that must be preceded and followed by a blank line are procedure
definitions. Elsewhere, blank lines should only be used where they aid readability.

#### End-of-file comments (3.3c)

We do not print out code for the purpose of code review. We do not require end-of-file comments.

#### Header files (4.2a)

Multiple source files corresponding to one header file are permitted if and only if they represent
alternative functionality that depends on a preprocessor definition.

#### Names of public data types (5.1c)

The names of all public data types shall be prefixed with `lf_`.

#### Names of public functions (6.1i)

The names of all public functions shall be prefixed with `lf_`.

#### Macro capitalization (6.1f)

The only acceptable use of macros containing lowercase letters is macros prefixed by `lf_` that
appear in the `reactor-c` public API.

#### Printed pages (6.2 and elsewhere)

We do not print code for the purpose of code review. In cases where Barr refers to the size of a
printed page, consider the size of a typical computer monitor instead. In particular, all reasonable
effort shall be taken to keep the length of each function limited to a maximum of **40 lines**.

#### Short variable names (7.1e)

We place no restrictions on the length of variable names; however, recommendations against cryptic
abbreviations and abbreviations not in a version-controlled table still apply.

#### Hungarian notation (7.1 j, k, l, m, n, o)

Hungarian notation is never required and is only permitted in unusual cases where it aids
readability. The abbreviations listed in the "Abbreviations" table remain valid. Example: The `p_`
and `pp_` prefixes are permitted in the navigation of potentially confusing data structures.

### Addenda

#### Inlining-related performance optimizations

Before attempting to encourage or force inlining, e.g. using the `inline` keyword and/or
parameterized macros, consider whether similar performance could be realized using link-time
optimization instead.

#### Return types in function declarations

The return type of a function must appear on the same line as the function name.

#### Line breaks

If the parenthesized expression(s) in a parameter list, `if` statement,`for` loop, `while loop`, or
similar is too long to fit on one line, then the opening parenthesis must be immediately followed by
a line feed, and the closing parenthesis and opening bracket must appear one their own line.
Example:

```
while (
    NULL != (
        current_reaction_to_execute = lf_sched_get_ready_reaction(worker_number)
    )
) {
    // Do something
}
```

#### Section headers in files

Sections within files shall not be marked by explicit section headers. They shall be made clear by
adhering to the sectioning suggested by the source and header file templates.

#### Documentation comment format

We use the Javadoc-style `/**` to mark documentation comments, and we precede any Doxygen commands
with an `@` sign. Example:

```c
/**
 * @brief Enqueue port absent reactions that will send a PORT_ABSENT
 * message to downstream federates if a given network output port is not present.
 */
```

The opening `/**` marker must be immediately followed by a line feed.

#### Documentation comment placement
Documentation comments of public procedures must be provided in the corresponding header files.

Documentation comments for nontrivial private procedures must be provided where those procedures are
implemented.

Duplication of multiline comments or of sigificant parts of multiline comments is forbidden.

## Abbreviations

The following is an extended version of the table provided by the Barr standard.

| Abbreviation | Meaning                       |
| ------------ | ----------------------        |
| adc          | analog-to-digital converter   |
| addr         | address                       |
| argc         | argument count                |
| argv         | argument vector               |
| avg          | average                       |
| b_           | boolean                       |
| buf          | buffer                        |
| cfg          | configuration                 |
| cond         | condition variable            |
| ctor         | constructor                   |
| curr         | current (item in a list)      |
| dac          | digital-to-analog converter   |
| dtor         | destructor                    |
| ee           | EEPROM                        |
| err          | error                         |
| fed          | federate/federated            |
| g_           | global                        |
| gpio         | general purpose I/0 pins      |
| h_           | handle (to)                   |
| id           | ID                            |
| init         | initialize                    |
| io           | input/output                  |
| ip           | Internet Protocol             |
| isr          | interrupt service routine     |
| lcd          | liquid crystal display        |
| led          | light-emitting diode          |
| lf           | Lingua Franca                 |
| max          | maximum                       |
| min          | minumum                       |
| msec         | millisecond                   |
| msg          | message                       |
| net          | network                       |
| next         | next (item in a list)         |
| nsec         | nanosecond                    |
| num          | number (of)                   |
| p_           | pointer (to)                  |
| param        | parameter                     |
| pp_          | pointer to a pointer (to)     |
| pqueue       | priority queue                |
| prev         | previous (item in a list)     |
| prio         | priority                      |
| ptag         | provisional tag               |
| pwm          | pulse width modulation        |
| q            | queue                         |
| ref          | reference                     |
| reg          | register                      |
| ret          | return value                  |
| rti          | runtime infrastructure        |
| rx           | receive                       |
| sched        | scheduler                     |
| sem          | semaphore                     |
| sta          | safe to advance (time)        |
| staa         | safe to assume absent         |
| stp          | safe to process               |
| str          | string (null-terminated)      |
| sync         | synchronize                   |
| tcp          | transmission control protocol |
| temp         | temperature                   |
| tmp          | temporary                     |
| tx           | transmit                      |
| udp          | User Datagram Protocol        |
| usec         | microsecond                   |
| util         | utilities                     |

## Source file template

```c
/**
 * @file
 * @author <author name>
 * @copyright See "LICENSE.md."
 */

<#includes for standard libraries and files associated with the target platform>

<#includes for header files belonging to our project>

<preprocessor definitions>

<definitions for private types>

<constants>

<global data definitions>

<static data declarations and definitions>

<private procedure prototypes>

<public procedure bodies>

<private procedure bodies>
```

## Header file template

```c
/**
 * @file
 * @brief <description of this file's purpose>
 * @author <author name>
 * @copyright See "LICENSE.md."
 */

#ifdef <file name in all caps, with "." replaced with "_">
#define <file name in all caps, with "." replaced with "_">

<#includes for standard libraries and files associated with the target platform>

<#includes for header files belonging to our project>

<preprocessor definitions>

<definitions for public types>

<global variable declarations>

<public procedure prototypes>

#endif // <file name in all caps, with "." replaced with "_">
```
