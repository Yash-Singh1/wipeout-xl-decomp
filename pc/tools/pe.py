"""Minimal PE32 reader: sections, base relocations, imports."""
import struct


class Section:
    def __init__(self, name, va, vsize, raw, rawsize, chars=0):
        self.name, self.va, self.vsize, self.raw, self.rawsize = name, va, vsize, raw, rawsize
        self.chars = chars

    @property
    def readonly_data(self):
        """initialised data that is neither writable nor executable"""
        return self.chars & 0x40 and not self.chars & 0xa0000000

    def __contains__(self, va):
        return self.va <= va < self.va + max(self.vsize, self.rawsize)


class PE:
    def __init__(self, path):
        self.path = path
        self.data = d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec, = struct.unpack_from('<H', d, pe + 6)
        optsize, = struct.unpack_from('<H', d, pe + 20)
        opt = pe + 24
        self.entry = struct.unpack_from('<I', d, opt + 16)[0]
        self.base = struct.unpack_from('<I', d, opt + 28)[0]
        self.entry += self.base
        ndirs = struct.unpack_from('<I', d, opt + 92)[0]
        self.dirs = [struct.unpack_from('<II', d, opt + 96 + 8 * i) for i in range(ndirs)]
        self.sections = []
        for i in range(nsec):
            o = opt + optsize + 40 * i
            name = d[o:o + 8].rstrip(b'\0').decode()
            vsize, va, rawsize, raw = struct.unpack_from('<IIII', d, o + 8)
            chars, = struct.unpack_from('<I', d, o + 36)
            self.sections.append(Section(name, va + self.base, vsize, raw, rawsize, chars))
        self.relocs = self._relocs()
        self.imports = self._imports()

    def section(self, name):
        return next(s for s in self.sections if s.name == name)

    def section_of(self, va):
        for s in self.sections:
            if va in s:
                return s
        return None

    def read(self, va, n):
        s = self.section_of(va)
        if s is None:
            return b'\0' * n
        off = va - s.va
        out = self.data[s.raw + off:s.raw + min(off + n, s.rawsize)]
        return out + b'\0' * (n - len(out))

    def u32(self, va):
        return struct.unpack('<I', self.read(va, 4))[0]

    def _relocs(self):
        rva, size = self.dirs[5]
        out = []
        p = rva + self.base
        end = p + size
        while p < end:
            page, bsize = struct.unpack('<II', self.read(p, 8))
            if bsize == 0:
                break
            for i in range((bsize - 8) // 2):
                e, = struct.unpack('<H', self.read(p + 8 + 2 * i, 2))
                if e >> 12 == 3:
                    out.append(page + self.base + (e & 0xfff))
            p += bsize
        return sorted(out)

    def cstr(self, va):
        b = self.read(va, 256)
        return b.split(b'\0', 1)[0].decode('latin-1')

    def _imports(self):
        rva, size = self.dirs[1]
        out = {}
        p = rva + self.base
        while True:
            olt, _, _, name, iat = struct.unpack('<IIIII', self.read(p, 20))
            if name == 0:
                break
            dll = self.cstr(name + self.base)
            i = 0
            while True:
                ent = self.u32((olt or iat) + self.base + 4 * i)
                if ent == 0:
                    break
                fn = ('#%d' % (ent & 0xffff)) if ent & 0x80000000 else self.cstr(ent + self.base + 2)
                out[iat + self.base + 4 * i] = (dll, fn)
                i += 1
            p += 20
        return out
