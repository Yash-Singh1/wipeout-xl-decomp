"""COFF object and archive (.lib) reader for MSVC 4.x objects."""
import struct

IMAGE_REL_I386_DIR32 = 6
IMAGE_REL_I386_DIR32NB = 7
IMAGE_REL_I386_REL32 = 0x14


class Sym:
    def __init__(self, idx, name, value, secnum, typ, cls, naux):
        self.idx, self.name, self.value, self.secnum = idx, name, value, secnum
        self.type, self.cls, self.naux = typ, cls, naux


class Obj:
    def __init__(self, data, name=''):
        self.name = name
        self.data = d = data
        machine, nsec, _, symptr, nsym, optsize, _ = struct.unpack_from('<HHIIIHH', d, 0)
        strtab = symptr + 18 * nsym
        self.strtab = d[strtab:]
        self.sections = []
        for i in range(nsec):
            o = 20 + optsize + 40 * i
            name = d[o:o + 8].rstrip(b'\0').decode('latin-1')
            if name.startswith('/'):
                name = self._str(int(name[1:]))
            vsize, va, size, raw, relptr, _, nrel, _, flags = struct.unpack_from('<IIIIIIHHI', d, o + 8)
            relocs = []
            for r in range(nrel):
                va_, sym, typ = struct.unpack_from('<IIH', d, relptr + 10 * r)
                relocs.append((va_, sym, typ))
            body = d[raw:raw + size] if raw else b'\0' * size
            self.sections.append(dict(name=name, size=size, data=body, relocs=relocs, flags=flags))
        self.syms = {}
        i = 0
        while i < nsym:
            o = symptr + 18 * i
            raw = d[o:o + 8]
            if raw[:4] == b'\0\0\0\0':
                name = self._str(struct.unpack_from('<I', raw, 4)[0])
            else:
                name = raw.rstrip(b'\0').decode('latin-1')
            value, sec, typ, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
            self.syms[i] = Sym(i, name, value, sec, typ, cls, naux)
            i += 1 + naux

    def _str(self, off):
        e = self.strtab.index(b'\0', off)
        return self.strtab[off:e].decode('latin-1')

    def functions(self):
        """yield (name, section index, offset, size) for code symbols."""
        bysec = {}
        for s in self.syms.values():
            if s.secnum > 0 and s.cls in (2, 3) and s.naux == 0 or (s.secnum > 0 and s.type == 0x20):
                sec = self.sections[s.secnum - 1]
                if sec['flags'] & 0x20:
                    bysec.setdefault(s.secnum - 1, set()).add((s.value, s.name, s.type))
        for si, ss in bysec.items():
            ss = sorted(ss)
            offs = sorted({v for v, _, _ in ss})
            for v, n, t in ss:
                if t != 0x20 and n.startswith('$'):
                    continue
                if n.startswith('.') or n.startswith('$L'):
                    continue
                nx = [o for o in offs if o > v]
                end = nx[0] if nx else self.sections[si]['size']
                yield n, si, v, end - v


def archive(path):
    """yield (member name, Obj) for COFF members of a .lib."""
    d = open(path, 'rb').read()
    assert d[:8] == b'!<arch>\n'
    p = 8
    longnames = b''
    while p + 60 <= len(d):
        hdr = d[p:p + 60]
        name = hdr[:16].decode('latin-1').strip()
        size = int(hdr[48:58].decode().strip())
        body = d[p + 60:p + 60 + size]
        if name == '//':
            longnames = body
        elif name not in ('/', ''):
            if name.startswith('/') and name[1:].isdigit():
                o = int(name[1:])
                e = longnames.find(b'\0', o)
                if e < 0:
                    e = longnames.find(b'/', o)
                name = longnames[o:e].decode('latin-1')
            name = name.rstrip('/')
            if body[:2] == b'\x4c\x01':
                yield name, Obj(body, name)
        p += 60 + size + (size & 1)
