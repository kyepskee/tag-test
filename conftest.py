# def pytest_generate_tests(metafunc):
#     if hasattr(metafunc.cls, 'inputs'):
#         new_inputs = []
#         for input in metafunc.cls.inputs:
#             new_inputs.append(tuple(map(lambda x : " " + x + " ", input)))
#         metafunc.parametrize("test_in,test_out,user_out", new_inputs)
