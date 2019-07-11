# Commit messages

Commit should follow the format:

```
SUBSYSTEM: WHAT HAS BEEN DONE IN IMPERATIVE AND PRESENT TENSE

DESCRIPTION OF THE COMMIT IN MORE DETAIL (optional though couraged)
```

## Subsystems
* arch
   * x86_64
   * i386
* build
* doc
* fs
* kernel
* mm
* sched
* sync

## Example

```
drivers: Add ability to install new output functions for tty

During boot, micael first uses VGA and then switches to VBE and
this is the cleanest way of switching between them
```
