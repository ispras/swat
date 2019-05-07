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
