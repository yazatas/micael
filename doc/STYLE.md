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

Static size arrays

```
int array[10] = { 0 };
```
