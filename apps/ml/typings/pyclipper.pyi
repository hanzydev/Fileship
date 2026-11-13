from collections.abc import Sequence

JT_ROUND: int
ET_CLOSEDPOLYGON: int

class PyclipperOffset:
    def AddPath(self, path: Sequence[Sequence[float]], join_type: int, end_type: int) -> None: ...
    def Execute(self, delta: float) -> list[list[list[int]]]: ...
