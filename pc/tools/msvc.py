"""Run MSVC 4.20 (CL.EXE) under Wine in a persistent Docker container.

The container mounts this directory at /w and the compiler (tools/msvc420, not included)
at /msvc. Wine keeps one wineserver alive, so a compile costs about half a second.
Build the image once with `docker build -t wipeoutxl-pc .`.
"""
import fcntl
import hashlib
import os, subprocess, sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
ROOT = REPO
MSVC = os.path.realpath(os.environ.get('XLPC_MSVC', os.path.join(REPO, 'tools', 'msvc420')))
IMAGE = os.environ.get('XLPC_IMAGE', 'wipeoutxl-pc')
NAME = os.environ.get('XLPC_CONTAINER', 'wipeoutxl-pc-' + hashlib.sha256(ROOT.encode()).hexdigest()[:8])

DEFAULT_FLAGS = ['/nologo', '/c', '/O2', '/W3']


def to_wine(path):
    p = os.path.abspath(path)
    if not p.startswith(ROOT + '/'):
        sys.exit('msvc.py: %s is outside %s' % (p, ROOT))
    return 'Z:\\w\\' + p[len(ROOT) + 1:].replace('/', '\\')


def to_container(path):
    return '/w/' + os.path.abspath(path)[len(ROOT) + 1:]


def ensure():
    # Parallel compiles share the container: serialize inspection and startup.
    key = hashlib.sha256((ROOT + '\0' + NAME).encode()).hexdigest()[:16]
    lock_path = os.path.join(tempfile.gettempdir(), 'xlpc-' + key + '.lock')
    with open(lock_path, 'a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        r = subprocess.run(['docker', 'inspect', '-f', '{{.State.Running}}', NAME],
                           capture_output=True, text=True)
        if r.returncode == 0:
            state = r.stdout.strip()
            if state == 'true':
                return
            if state != 'false':
                raise RuntimeError('docker inspect returned an unknown state: ' + state)
            subprocess.run(['docker', 'start', NAME], check=True, capture_output=True)
            return
        if 'no such object: ' + NAME.lower() not in r.stderr.lower():
            raise RuntimeError('cannot inspect compiler container: ' + r.stderr.strip())
        subprocess.run(['docker', 'run', '-d', '--name', NAME, '--platform', 'linux/amd64',
                        '-v', '%s:/w' % ROOT, '-v', '%s:/msvc:ro' % MSVC, IMAGE,
                        'sh', '-c', 'wineserver -p -f'], check=True, capture_output=True)


def compile(src, obj, flags=None, includes=()):
    """Compile src to obj. Returns (ok, compiler output)."""
    ensure()
    inc = ';'.join([to_wine(i) for i in includes] + ['Z:\\msvc\\include'])
    args = list(flags or DEFAULT_FLAGS) + ['/Fo' + to_wine(obj), to_wine(src)]
    if os.path.exists(obj):
        os.unlink(obj)
    cmd = ['docker', 'exec', '-e', 'INCLUDE=' + inc, '-w', to_container(os.path.dirname(os.path.abspath(src))),
           NAME, 'wine', '/msvc/bin/CL.EXE'] + args
    r = subprocess.run(cmd, capture_output=True, text=True, errors='replace')
    out = '\n'.join(l for l in (r.stdout + r.stderr).replace('\r', '').splitlines()
                    if l.strip() and not l.strip().endswith('.c') and 'fixme:' not in l)
    return os.path.exists(obj) and r.returncode == 0, out


if __name__ == '__main__':
    ok, out = compile(sys.argv[1], sys.argv[2], sys.argv[3:] or None,
                      [os.path.join(REPO, 'include')])
    print(out)
    sys.exit(0 if ok else 1)
