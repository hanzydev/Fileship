import pytest
from harness import Run, measure, report

from fileship_ml.presets import PRESETS


@pytest.fixture(scope='session', params=list(PRESETS))
def run(request: pytest.FixtureRequest) -> Run:
    return measure(request.param)


def pytest_terminal_summary(terminalreporter: pytest.TerminalReporter) -> None:
    for line in report:
        terminalreporter.write_line(line)
