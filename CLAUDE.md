# Hatchling Platform

## Tags

Always grep the `tag` file as shown below to find symbol definitions. If the
`tags` file doesn't exist, generate it first.

Generate:

```bash
ctags -R --fields=+n -f tags .
```

Find a symbol definition (with line numbers):

```bash
grep $'^<symbol>\t' tags
```
