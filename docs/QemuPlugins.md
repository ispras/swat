## Plugins for guest code instrumentation

Our instrumentation subsystem exploits TCG helper mechanism to embed
callbacks into the translation blocks. These callbacks may be inserted
before the specific instructions.

We added new instrumentation API for QEMU which includes the following parts:
* some translator modifications to enable instrumenting the instructions
* dynamic binary instrumentation part
* subsystem for dynamically loaded plugins that interact with this API

The aim of the instrumentation is implementing different runtime
tracers that can track the executed instructions, memory and
hardware operations.

The plugins do not have much dependencies from the QEMU
core. They are be built as a separate projects using just
a couple of the headers.

## API for the plugins

### Functions that plugin should provide

#### bool plugin_init(const char *args)

Initializes plugin data structures. Should return true if plugin was successfully initialized.

Arguments are passed to the plugin though the command line: -plugin file=$dir/libplugin.so,args="List of the arguments"

#### bool plugin_needs_before_insn(uint64_t pc, void *cpu)

Returns true if the current location should be instrumented - plugin_before_insn should be called.

This function may be called in translations and runtime phases.

#### void plugin_before_insn(uint64_t pc, void *cpu)

Called at the locations (in runtime) that were approved by plugin_needs_before_insn function.

#### bool plugin_needs_before_tb(uint64_t pc, void *cpu)

Returns true if the current translation block should be instrumented - plugin_before_tb should be called.

#### void plugin_before_tb(uint64_t pc, void *cpu)

Called for the beginning of the translation blocks (in the translation phase)
hat were approved by plugin_needs_before_tb function.

### Function of QEMU core available for plugins

#### void qemulib_log(const char *fmt, ...)

Function for logging any data in the standard QEMU log, specified by -D option.

#### int qemulib_read_memory(void *cpu, uint64_t addr, uint8_t *buf, int len)

Function for reading the guest memory from the plugins. Returns non-zero in case of the failure.

#### int qemulib_read_register(void *cpu, uint8_t *mem_buf, int reg)

Function for reading the value of the specific register.

Register IDs are specified in qemu/plugins/include/regnum.h header file.
