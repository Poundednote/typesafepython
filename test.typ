a: int = 2
a = 2
a + b
a - b
a * b
a / b
a // b
a % b
a << 5
b = 5 >> 2
a + b
return a
global a
global a + b
nonlocal a - b


block:
    | NEWLINE INDENT simple_stmts:
    | simple_stmt !';' NEWLINE  # Not needed, there for speedup
    | ';'.simple_stmt+ [';'] NEWLINE  DEDENT
