from typing import List

a: str = "new_drank"
b: str = "string"
c: str = ""
d: List[int | str | float | bool] = [1, 2, 3, 4, "hello", 1.0, "string"]

class Foo:
    baz: str = "hello"

    def bar(self: int) -> int:
        return 1

def first() -> str:
    while 1:
        return a

    if a + b:
        return b
    else:
        1 + 1
        return "str"

def second() -> int:
    foo: Foo
    return 1

def cawk(a: int) -> int:
    return a + 1

cawk(d[1])
