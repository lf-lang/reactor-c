This sub-project defines the platform abstraction used by reactor-c.

It exposes an interface that does include compile-time constructs such as
typedefs and preprocessor definitions. Use the `platform` subproject if the
simplified interface that appears there is sufficient.

Strongly prefer to depend on the `platform` subproject if the module you are
building needs to be compiled using a separate toolchain.
