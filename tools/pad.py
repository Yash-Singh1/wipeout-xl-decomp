import sys
path, size = sys.argv[1], int(sys.argv[2], 0)
data = open(path, "rb").read()
if len(data) > size:
    sys.exit(f"{path} is {len(data):#x} bytes, larger than {size:#x}")
open(path, "wb").write(data + b"\0" * (size - len(data)))
