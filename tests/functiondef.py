def noargs() -> int:
    3 + 3
    2 + 2
    return 3+2

def func(a: int = 1, poo: str = "a") -> str:
    "hello" + "hi"
    return "a"

def func(a: int, b: str = "a", *args, **kwargs) -> str:
    return "hello"
