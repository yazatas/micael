# Coding style

Indent by 4 spaces (no tabs).

## Comments

Use `/* comment */` instead of `// comment`

For multiline comments use the following format:

```
/* blabla
 * 
 * blabla, empty lines have * too!
 * blabla
 * 
 * blabla */

```

If commenting large blocks of code, use:

```
#if 0

#endif
```

## Functions

Both curly brackets have their own line. Unused parameters should be casted to void.

```
int func(int param)
{
    (void)param;
    return 0;
}
```

Static functions should be prefixed with `__` to make them more visible in the code.
The "namespace" should be dropped for static functions. For example (this function is part of page.c, namespace mmu):

```
static void __claim_range(mm_zone_t *zone, unsigned long start, unsigned long end)
```

## Whitespacing

More is more

```
if (expression) {

}

while (expression) {

}

do {

} while (expression);

for (int i = 0; i < 10; ++i) {

}
```

If the condition of if/while takes more than one line, use the following bracket style:

```
if ((sb        = kmalloc(sizeof(superblock_t))) == NULL ||
    (sb->s_ops = kmalloc(sizeof(super_ops_t)))  == NULL)
{
    /* error handling */
}

```

Equal signs of assignments should be aligned if feasible. For example
```
file->f_mode   = mode;
file->f_dentry = dntr;
file->f_ops    = dntr->d_inode->i_fops;
```

It's not strictly necessary if the variable names are of very different lengths.

## Switch

Empty line after each case

```
switch (expression) {
    case a:
        break;

    case b:
        break;

    case c:
        break;
}
```

## Misc

Use whitespaces around curly brackets

```
int array[10] = { 0 };
```

All "impossible" conditions should be checked using kassert(). Possible, but invalid conditions should be checked normally and an error message should be printed and errno should be set/returned.


Ignoring the return value is discouraged but if it's done for whatever reason, the function call must be casted to void:
```
(void)mmu_slab_preinit();
```
