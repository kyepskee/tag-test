import subprocess
import tempfile
import sys

import pytest

FLAGS: list[str] = ["-std=c++23"]

@pytest.fixture(scope="session")
def compile():
    subprocess.run(
        args=["g++", *FLAGS, "touchk.cpp", "-o", "checker"]
    )

def run(test_in: str, user_out: str, test_out: str):
    test_in_file = tempfile.NamedTemporaryFile(delete=False)
    test_out_file = tempfile.NamedTemporaryFile(delete=False)
    user_out_file = tempfile.NamedTemporaryFile(delete=False)
    
    test_in_file.write(test_in.encode())
    test_out_file.write(test_out.encode())
    user_out_file.write(user_out.encode())

    for f in [test_in_file, test_out_file, user_out_file]:
        f.close()

    ret = subprocess.run(
        ["./checker", test_in_file.name, user_out_file.name, test_out_file.name],
        capture_output=True)

    return bytes(ret, encoding='utf8')

class TestCorrect():
    inputs = [("1\n2 1\n1 2 1", "NO", "NO")]

    @pytest.mark.parametrize("test_in,user_out,test_out", inputs)
    def test_correct(self, compile, test_in, test_out, user_out):
        assert run(test_in, test_out, user_out).stdout == b'OK\n\n100\n'

class TestWrong():
    inputs = [("1\n2 1\n1 2 1", "NO", "no"), ("1\n2 1\n1 2 1", "NO", "YES\n1 1")]

    @pytest.mark.parametrize("test_in,user_out,test_out", inputs)
    def test_wrong(self, compile, test_in, test_out, user_out):
        assert run(test_in, test_out, user_out).stdout == 'WRONG\n\n0\n'

def wrong_message(line, pos, message, score = 0):
    return bytes(f"WRONG\nWiersz {line}, pozycja {pos}: {message}\n{score}\n", encoding='utf8')

def wrong_out_of_range(line, pos, score = 0):
    return wrong_message(line, pos, "Liczba calkowita spoza zakresu") 

def wrong_expected(line, pos, char, expected, score = 0):
    return wrong_message(line, pos, f"Wczytano '{char}', oczekiwano {expected}")

class TestVerbatim():
    inputs = [
            (
"""2
2 1
1 2 1
2 1
1 2 1""",
                "NO\nNO",
                "NO\n\nNO",
                wrong_expected(line=2, pos=1, char="\\n", expected="napisu")
            )]

    @pytest.mark.parametrize("test_in,user_out,test_out,checker_out", inputs)
    def test_verbatim(self, compile, test_in, test_out, user_out, checker_out):
        assert run(test_in, test_out, user_out).stdout == checker_out
