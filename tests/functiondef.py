def noargs() -> int:
    3 + 3
    2 + 2
    return 3+2

def func(a: int = 1, poo: string) -> str:
    return "hello"

def func(a: int = 1, b: string, *args, **kwargs) -> str:
    return "hello"
