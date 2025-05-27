import subprocess
import tempfile
import sys
# from hypothesis import given, strateges as st

# @st.composite
# def

import pytest

FLAGS: list[str] = ["-std=c++23"]

@pytest.fixture(scope="session")
def compile():
    print('test')
    subprocess.run(
        args=["g++", *FLAGS, "touchk.cpp", "-o", "checker"]
    )

def run(test_in: str, test_out: str, user_out: str):
    test_in_file = tempfile.NamedTemporaryFile(delete=False)
    test_out_file = tempfile.NamedTemporaryFile(delete=False)
    user_out_file = tempfile.NamedTemporaryFile(delete=False)
    
    test_in_file.write(test_in.encode())
    test_out_file.write(test_out.encode())
    user_out_file.write(user_out.encode())
    print(test_in_file)
    print(test_in_file.name)

    for f in [test_in_file, test_out_file, user_out_file]:
        f.close()

    ret = subprocess.run(["./checker", test_in_file.name, test_out_file.name, user_out_file.name], capture_output=True)

    return ret

class TestCorrect():
    inputs = [("1\n2 1\n1 2 1", "NO", "NO")]

    @pytest.mark.parametrize("test_in,test_out,user_out", inputs)
    def test_correct(self, compile, test_in, test_out, user_out):
        assert run(test_in, test_out, user_out).stdout == b'OK\n\n100\n'

class TestWrong():
    inputs = [("1\n2 1\n1 2 1", "NO", "no"), ("1\n2 1\n1 2 1", "NO", "YES\n1 1")]

    @pytest.mark.parametrize("test_in,test_out,user_out", inputs)
    def test_wrong(self, compile, test_in, test_out, user_out):
        assert run(test_in, test_out, user_out).stdout == b'WRONG\n\n0\n'
        
def wrong_message(line, pos, message, score = 0):
    return f"WRONG\nWiersz {line}, pozycja {pos}: {message}"

def wrong_out_of_range(line, pos, score = 0):
    return wrong_message(line, pos, "Liczba calkowita spoza zakresu") 

def wrong_expected(line, pos, char, expected, score = 0):
    return wrong_message(line, pos, "Wczytano '{char}', oczekiwano {expected}")

class TestVerbatim():
    inputs = [
            (
"""1
3 4
1 2 1
2 3 2
3 2 3
2 1 1""",
                "YES\n2 2 3",
                "\nYES\n2 3 2",
                wrong_expected(1, 1, "\\n", "napisu")
            )]

    @pytest.mark.parametrize("test_in,test_out,user_out,checker_out", inputs)
    def test_verbatim(self, compile, test_in, test_out, user_out, checker_out):
        assert run(test_in, test_out, user_out).stdout == checker_out

def main():
    A="""1
    2 1
    1 2 1
    """
    B="NO"
    C="NO"
    res = run(A, B, C)
    print(res.stdout)

if __name__ == '__main__':
    sys.exit(main())
