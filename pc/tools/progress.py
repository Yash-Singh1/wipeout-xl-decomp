"""Build every src/*.c and check it against the exe.

Default build: every function compiled must match the exe byte for byte and
every object's data must agree with the exe. Drafts that don't match yet live
under `#ifdef NON_MATCHING` and are only compiled with --nonmatching.

usage: progress.py [--nonmatching] [--target powervr|net|launcher|dxlink] [-j N] [FILES...]
--target powervr additionally compares every C function with POWERVR.EXE and lists
the D3D-matched ones that differ there; OK/FAIL and the first count stay the D3D result.
--target net does the same against NET-WOXL.EXE (compiled with /DNETPLAY and config/net/flags.tsv);
the files under src/net/ must match, shared files only report MATCH/EQUIV/DIFF/missing.
prints OK / FAIL and the matched share of the game code.
"""
import sys, os, re, glob, argparse, concurrent.futures as cf
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import msvc, symbols as S
from coff import Obj
from check import Checker, entropy_search
from datarefs import GAME_END

REPO = S.REPO



# full-tier proofs of shared functions in PowerVR/NET runs only up to this size (a full tier on every large
# function made a NET run exceed 40 minutes; the D3D gate gives its own functions the full proof)
SHARED_FULL_PROOF_MAX = 600
# A quick proof of a large, different renderer body can itself take minutes.
# Keep byte retries for those bodies; reserve repeated proofs for smaller
# functions, exact executable pairs, and explicit target implementations.
SHARED_ENTROPY_PROOF_MAX = 4096

def target_only_functions(src, define):
    """names of the functions whose definition sits inside an open `#ifdef <define>` (or `#if defined(<define>)`):
    bodies written for that target only, which the D3D gate never proves"""
    out, stack = set(), []
    for line in open(src, errors='replace'):
        d = re.match(r'\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)', line)
        if d:
            kw, rest = d.group(1), d.group(2)
            if kw in ('ifdef', 'ifndef', 'if'):
                stack.append(kw != 'ifndef' and re.search(r'\b%s\b' % define, rest) is not None
                             and '!' not in rest)
            elif kw in ('elif', 'else') and stack:
                stack[-1] = False
            elif kw == 'endif' and stack:
                stack.pop()
            continue
        if any(stack):
            m = re.match(r'[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;]*$', line)
            if m and not line.startswith((' ', '\t')) and m.group(1) not in ('if', 'while', 'for', 'switch', 'return'):
                out.add(m.group(1))
    return out


def equiv_block_functions(src):
    """names of the functions defined inside top-level #ifdef EQUIV blocks"""
    from promote_equiv import blocks, funcs_in
    lines, bl = blocks(open(src, errors='replace').read())
    out = set()
    for st, en, g in bl:
        if g.replace(' ', '') in ('#ifdefEQUIV', '#ifEQUIV', '#ifdefined(EQUIV)'):
            out |= set(funcs_in(lines, st, en))
    return out


def build_equiv(src, target='d3d'):
    tag = target + '/equiv' if target != 'd3d' else 'equiv'
    obj = os.path.join(REPO, 'build', tag, os.path.relpath(src, os.path.join(REPO, 'src'))[:-2] + '.obj')
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    flags = S.flags_for(src, target) + ['/DEQUIV', '/DNON_MATCHING']
    if target in S.TARGET_DEFINES:
        flags.append(S.TARGET_DEFINES[target])
    ok, out = msvc.compile(src, obj, flags, [os.path.join(REPO, 'include'), os.path.dirname(src)])
    return src, ok, out, obj


def build(src, nonmatching, target='d3d'):
    tag = (target + '/' if target != 'd3d' else '') + ('nm' if nonmatching else 'default')
    obj = os.path.join(REPO, 'build', tag, os.path.relpath(src, os.path.join(REPO, 'src'))[:-2] + '.obj')
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    flags = S.flags_for(src, target) + (['/DNON_MATCHING'] if nonmatching else [])
    if target in S.TARGET_DEFINES:
        flags.append(S.TARGET_DEFINES[target])
    ok, out = msvc.compile(src, obj, flags, [os.path.join(REPO, 'include'), os.path.dirname(src)])
    return src, ok, out, obj


def equiv_entropy_search(ck, src, flags, includes, names, judge, quick_judge, initial):
    # Exhausting every proof fallback at each failed layout can take minutes
    # before reaching a layout the first tier proves. Try a bounded batch
    # cheaply, then retain the complete search for every unresolved function.
    # No failure in the quick batch is final.
    found = entropy_search(ck, src, flags, includes, names,
                           kmax=24, judge=quick_judge, tag='_equiv')
    remaining = [name for name in names if name not in found]
    # The default layout may need a later proof tier. Try its complete proof
    # before the complete entropy search, just as the original gate did.
    remaining = [name for name in remaining if not initial(name)]
    if remaining:
        found.update(entropy_search(ck, src, flags, includes, remaining,
                                    judge=judge, tag='_equiv'))
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--nonmatching', action='store_true')
    ap.add_argument('-j', type=int, default=int(os.environ.get('JOBS', 6)))
    ap.add_argument('-q', action='store_true')
    ap.add_argument('--target', choices=('d3d', 'powervr', 'net', 'launcher', 'dxlink'), default='d3d')
    ap.add_argument('--update-baseline', action='store_true',
                    help='after a full non-D3D run with no losses, record its verified set in config/<target>/verified.txt')
    ap.add_argument('files', nargs='*')
    a = ap.parse_args()
    if a.target in ('launcher', 'dxlink'):
        target_progress(a)
        return
    files = [os.path.abspath(f) for f in a.files] or sorted(glob.glob(os.path.join(REPO, 'src', '**', '*.c'), recursive=True))
    # each target's own directory holds sources that exist only in that exe
    others = {'d3d': ('powervr', 'net'), 'powervr': ('net',), 'net': ('powervr',)}[a.target] + ('launcher', 'dxlink')
    files = [f for f in files if not any(f.startswith(os.path.join(REPO, 'src', d) + os.path.sep) for d in others)]
    own = os.path.join(REPO, 'src', a.target) + os.path.sep      # directory of the sources that exist only in this target
    files = [os.path.abspath(f) for f in files]
    msvc.ensure()
    ck = Checker(target=a.target)
    if a.target != 'd3d':
        funcs = {r[0]: r[1] for r in S.load_func_rows(S.pv_paths(a.target)[1]) if r[0] < S.game_end(a.target)}
    else:
        funcs = {r[0]: r[1] for r in S.load_func_rows() if r[0] < GAME_END}
    total_b = sum(funcs.values())
    matched = {}
    drafts = {}
    bad = []
    noisy = []
    pv_missing = []
    pv_diffs = []
    pv_exact = {int(f[1], 16) for l in open(S.pv_paths()[2])
                if not l.startswith('#') and l.rstrip().endswith('\texact')
                for f in [l.split('\t')]} if a.target == 'powervr' else set()
    shared_diff_info = []      # shared-file functions that differ in this target's build: (src, rel, name, addr, info)
    shared_ok = a.target == 'net'     # NET shared sources are built with other flags: a DIFF there is reported, not an error
    with cf.ThreadPoolExecutor(a.j) as ex:
        results = list(ex.map(lambda f: build(f, a.nonmatching, a.target), files))
    for src, ok, out, obj in results:
        rel = os.path.relpath(src, REPO)
        if not ok:
            bad.append('%s: compile failed\n%s' % (rel, out))
            continue
        o = Obj(open(obj, 'rb').read(), obj)
        res, learned, base = ck.check(o)
        lits = dict(ck.lits)
        diffs = [r[0] for r in res if r[2] == 'DIFF']
        if a.target == 'powervr':
            pv_bodies = target_only_functions(src, 'POWERVR')
            # Similar pairs can also differ only in compiler heap state. Size is
            # a retry filter; a successful retry still compares every byte.
            diffs = [name for name, addr, status, info in res if status == 'DIFF'
                     and (addr in pv_exact or name in pv_bodies or len(info[0]) == len(info[1]))]
        ent = {}
        if diffs and not a.nonmatching:
            ent = entropy_search(ck, src, S.flags_for(src, a.target) + ([S.TARGET_DEFINES[a.target]] if a.target in S.TARGET_DEFINES else []), [os.path.join(REPO, 'include'), os.path.dirname(src)], diffs, kmax=32 if a.target == 'net' else 96)
            ck.lits = lits
        for name, addr, status, info in res:
            if name in ent:
                status = 'MATCH'
                noisy.append('%s: %s (k=%d)' % (rel, name, ent[name]))
            if status == 'NOADDR':
                if a.target != 'd3d':
                    pv_missing.append((rel, name))
                    if src.startswith(own):
                        bad.append('%s: %s has no %s address' % (rel, name, a.target))
                else:
                    bad.append('%s: %s has no address' % (rel, name))
            elif status == 'MATCH':
                if addr in matched:
                    bad.append('%s: %s %06x already matched in %s' % (rel, name, addr, matched[addr]))
                matched[addr] = rel
            elif a.nonmatching:
                drafts[addr] = (rel, name)
            elif a.target == 'powervr' and not src.startswith(own) and addr not in pv_exact:
                pv_diffs.append((rel, name, addr))
                shared_diff_info.append((src, rel, name, addr, info))
            elif shared_ok and not src.startswith(own):
                pv_diffs.append((rel, name, addr))
                shared_diff_info.append((src, rel, name, addr, info))
            else:
                bad.append('%s: %s %06x does not match' % (rel, name, addr))
        for n, ad in learned.items():
            known = ck.syms.by_addr.get(ad)
            if known and known != n and not a.nonmatching:
                bad.append('%s: symbol %s resolves to %06x, which symbols.tsv names %s' % (rel, n, ad, known))
        for sec, b, size, diffs in ck.data_check(o, base):
            if diffs and not a.nonmatching and not (shared_ok and not src.startswith(own)):
                bad.append('%s: %s data at %06x differs in %d of %d bytes' % (rel, sec, b, len(diffs), size))
    # volatile casts and volatile locals only coax the compiler into the exe's code
    # shape; they are not how the original was written (top-level declarations of
    # globals a timer or another thread updates are fine)
    for f in files:
        for n, line in enumerate(open(f, errors='replace'), 1):
            if re.search(r'\(\s*volatile\b', line) or re.match(r'\s+volatile\b', line):
                bad.append('%s:%d: volatile used to steer code generation: %s' % (os.path.relpath(f, REPO), n, line.strip()))
            # the EQUIV pass only sees top-level '#ifdef EQUIV' blocks: a guard that also mentions a
            # target hides its functions from the D3D gate (nest '#ifdef NETPLAY' inside instead)
            if re.match(r'\s*#\s*elif\b.*\bEQUIV\b', line) or (
                    re.match(r'\s*#\s*if\b.*\bEQUIV\b', line) and re.search(r'\b(NETPLAY|POWERVR)\b', line)):
                bad.append('%s:%d: EQUIV block guard mixed with a target define: %s' % (os.path.relpath(f, REPO), n, line.strip()))
    # one flags line per file
    seen = {}
    flags_path = S.target_flags_path(a.target) if a.target != 'd3d' else S.FLAGS
    for l in open(flags_path if os.path.exists(flags_path) else S.FLAGS):
        if l.startswith('#') or not l.strip():
            continue
        f = l.split('\t')[0]
        if f in seen:
            bad.append('config/flags.tsv: %s is listed more than once' % f)
        seen[f] = 1
    # symbols.tsv must name each address once and give each name one address
    names, addrs = {}, {}
    for ad, n, k in ck.syms.rows:
        if k == 'lib':
            continue
        names.setdefault(ad, set()).add(n)
        addrs.setdefault(n, set()).add(ad)
    for ad, ns in sorted(names.items()):
        if len(ns) > 1:
            bad.append('config/symbols.tsv: %06x has several names: %s' % (ad, ', '.join(sorted(ns))))
    for n, ads in sorted(addrs.items()):
        if len(ads) > 1:
            bad.append('config/symbols.tsv: %s is given several addresses: %s' % (n, ', '.join('%06x' % x for x in sorted(ads))))
    # EQUIV pass: files with #ifdef EQUIV blocks are rebuilt with /DEQUIV /DNON_MATCHING
    # (the same build the drafts were proven in); every function inside an EQUIV
    # block must then be a byte match or proven equivalent (tools/equiv.py)
    proven = {}
    exact_in_equiv = []
    oa_proofs = []
    if not a.nonmatching:
        import equiv
        from equivcheck import void_functions
        equiv.configure(ck.exe, ck.funcs)
        # a shared function the target builds differently: one proof attempt (quick tier, then the full tier for
        # functions up to SHARED_FULL_PROOF_MAX bytes; no heap-noise retries; informational, as these never fail
        # the target run). Counted as EQUIV for the target.
        for src, rel, name, addr, info in shared_diff_info:
            equiv.for_file(src, a.target)
            rv = name not in void_functions(src)
            good, why = equiv.equivalent(info[0], info[1], addr, returns_value=rv, quick=True)
            if not good and len(info[1]) <= SHARED_FULL_PROOF_MAX:
                good, why = equiv.equivalent(info[0], info[1], addr, returns_value=rv)
            if good:
                proven[addr] = rel
                pv_diffs[:] = [d for d in pv_diffs if d[2] != addr]
        eq_files = [f for f in files if re.search(r'^#\s*if(def)?\s*\(?\s*(defined\s*\(\s*)?EQUIV\b', open(f, errors='replace').read(), re.M)]
        with cf.ThreadPoolExecutor(a.j) as ex:
            eq_results = list(ex.map(lambda f: build_equiv(f, a.target), eq_files))
        for src, ok, out, obj in eq_results:
            rel = os.path.relpath(src, REPO)
            if not ok:
                bad.append('%s: EQUIV build failed\n%s' % (rel, out))
                continue
            voids = void_functions(src)
            in_equiv = equiv_block_functions(src)
            equiv.for_file(src, a.target)
            res, _, _ = ck.check(Obj(open(obj, 'rb').read(), obj))
            failed = {}
            initial_diffs = {}
            proof_results = {}

            def proof(name, addr, info, quick=False):
                # Entropy counts often produce identical machine code. The
                # file, oracles and return-value contract stay fixed throughout
                # this search, so each byte pair needs only one complete proof.
                key = addr, info[0], info[1], name not in voids, quick
                if key not in proof_results:
                    proof_results[key] = equiv.equivalent(
                        info[0], info[1], addr, returns_value=name not in voids, quick=quick)
                return proof_results[key]

            # Shared functions usually get a bounded proof against the other exe.
            # PowerVR pairs with identical masked bodies also get the full proof:
            # the size limit otherwise omits large, already provable routines such
            # as SfxBuildMixTables. This changes proof coverage, not proof rules.
            shared = a.target != 'd3d' and not src.startswith(own)
            # bodies written for this target only get the full proof at any size (the size limit exists
            # for the many shared functions the D3D gate already proves)
            target_only = target_only_functions(src, S.TARGET_DEFINES[a.target][2:]) \
                if shared and a.target in S.TARGET_DEFINES else set()
            shared_unproven = {}
            for name, addr, status, info in res:
                if addr in matched or name not in in_equiv:
                    continue        # only #ifdef EQUIV blocks are claimed (drafts are built too)
                if status == 'MATCH':
                    proven[addr] = rel      # byte-exact in the EQUIV build (could leave its guard)
                    exact_in_equiv.append('%s: %s' % (rel, name))
                    continue
                if shared:
                    # one proof attempt, as for the shared default-build functions (informational)
                    good = status == 'DIFF' and (proof(name, addr, info, quick=True)[0] or
                                                 ((len(info[1]) <= SHARED_FULL_PROOF_MAX or name in target_only) and
                                                  proof(name, addr, info)[0]))
                    if good:
                        proven[addr] = rel
                    elif status == 'DIFF' and a.target in ('net', 'powervr'):
                        shared_unproven[name] = addr
                    continue
                if status == 'NOADDR':
                    bad.append('%s: %s has no address (EQUIV build)' % (rel, name))
                    continue
                good, why = proof(name, addr, info, quick=True)
                if good:
                    proven[addr] = rel
                    if 'assuming no aliasing' in why:
                        oa_proofs.append('%s: %s' % (rel, name))
                else:
                    failed[name] = (addr, why)
                    initial_diffs[name] = info
            if shared_unproven:
                # Try the cheap heap layouts before a full proof of a large
                # PowerVR pair. An unlucky register layout can make the full
                # proof much more expensive than a successful quick retry.
                def shared_judge(name, addr, status, info):
                    return status == 'MATCH' or (a.target == 'powervr' and status == 'DIFF' and
                                                 (len(info[0]) <= SHARED_ENTROPY_PROOF_MAX or
                                                  addr in pv_exact or name in target_only) and
                                                 proof(name, addr, info, quick=True)[0])
                flags = S.flags_for(src, a.target) + ['/DEQUIV', '/DNON_MATCHING', S.TARGET_DEFINES[a.target]]
                ent = entropy_search(ck, src, flags, [os.path.join(REPO, 'include'), os.path.dirname(src)],
                                     list(shared_unproven), kmax=96 if a.target == 'powervr' else 24,
                                     judge=shared_judge if a.target == 'powervr' else None,
                                     tag='_equivpowervr' if a.target == 'powervr' else '_equivnet')
                for name, k in sorted(ent.items()):
                    proven[shared_unproven[name]] = rel
                    noisy.append('%s: %s (EQUIV build, verified, k=%d)' % (rel, name, k))
                for name, addr, status, info in res:
                    if name in shared_unproven and name not in ent and addr in pv_exact:
                        if proof(name, addr, info)[0]:
                            proven[addr] = rel
                # Target bodies may also prove equivalent at another heap state.
                # Keep these retries on the quick tier.
                rest = [n for n, addr in shared_unproven.items()
                        if n not in ent and addr not in proven and n in target_only]
                if rest:
                    def target_judge(name, addr, status, info):
                        # quick tier only: a full proof per heap state is far too slow for large bodies
                        return status == 'MATCH' or (status == 'DIFF' and proof(name, addr, info, quick=True)[0])
                    ent2 = entropy_search(ck, src, flags, [os.path.join(REPO, 'include'), os.path.dirname(src)],
                                          rest, kmax=24, judge=target_judge,
                                          tag='_equivpowervrp' if a.target == 'powervr' else '_equivnetp')
                    for name, k in sorted(ent2.items()):
                        proven[shared_unproven[name]] = rel
                        noisy.append('%s: %s (EQUIV build, proven, k=%d)' % (rel, name, k))
            if failed:
                # compiler heap noise, as for MATCH*: a header change elsewhere can shift
                # the allocation; retry with dummy declarations in front
                def judge(name, addr, status, info):
                    return status == 'MATCH' or (status == 'DIFF' and proof(name, addr, info)[0])
                def quick_judge(name, addr, status, info):
                    return status == 'MATCH' or (status == 'DIFF' and proof(name, addr, info, quick=True)[0])
                def full_initial(name):
                    addr, _ = failed[name]
                    good, why = proof(name, addr, initial_diffs[name])
                    if good:
                        proven[addr] = rel
                        if 'assuming no aliasing' in why:
                            oa_proofs.append('%s: %s' % (rel, name))
                    else:
                        failed[name] = (addr, why)
                    return good
                flags = S.flags_for(src, a.target) + ['/DEQUIV', '/DNON_MATCHING']
                if a.target in S.TARGET_DEFINES:
                    flags.append(S.TARGET_DEFINES[a.target])
                includes = [os.path.join(REPO, 'include'), os.path.dirname(src)]
                ent = equiv_entropy_search(ck, src, flags, includes, list(failed),
                                           judge, quick_judge, full_initial)
                for name, (addr, why) in sorted(failed.items()):
                    if addr in proven:
                        continue
                    if name in ent:
                        proven[addr] = rel
                        noisy.append('%s: %s (EQUIV, k=%d)' % (rel, name, ent[name]))
                    else:
                        bad.append('%s: %s %06x in the EQUIV build is neither exact nor equivalent: %s' % (rel, name, addr, why))
    mb = sum(funcs.get(x, 0) for x in matched)
    pb = sum(funcs.get(x, 0) for x in proven)
    if noisy:
        print('%d functions match only with dummy declarations in front (compiler heap noise):' % len(noisy))
        for l in noisy:
            print('  ' + l)
    if oa_proofs:
        print('%d EQUIV proofs rely on the files\' /Oa or /Ow contract (no object is reached through two names: pointer accesses touch neither named globals nor other pointers\' objects):' % len(oa_proofs))
        for l in oa_proofs:
            print('  ' + l)
    if exact_in_equiv:
        print('%d functions in #ifdef EQUIV blocks match byte for byte in that build:' % len(exact_in_equiv))
        for l in exact_in_equiv:
            print('  ' + l)
    for l in bad:
        print(l)
    print('%d/%d game functions match (%.1f%% of %d bytes)' % (
        len([x for x in matched if x in funcs]), len(funcs), 100.0 * mb / total_b, total_b))
    if a.target != 'd3d':
        known = set(matched) | set(proven)
        diff_addrs = {addr for _, _, addr in pv_diffs}
        missing_addrs = set(funcs) - known - diff_addrs
        print('%s function status: %d MATCH/EQUIV, %d different mapped functions, %d missing' % (
            'NET' if a.target == 'net' else 'PowerVR', len(known & set(funcs)), len(diff_addrs), len(missing_addrs)))
        diff_names = {n for _, n, _ in pv_diffs}
        for addr in sorted(funcs):
            name = ck.syms.by_addr.get(addr, 'func_%06x' % addr)
            if addr in matched:
                state = 'MATCH'
            elif addr in proven:
                state = 'EQUIV'
            elif addr in diff_addrs or name in diff_names:
                state = 'DIFF'
            else:
                state = 'missing'
            print('  %06x %-8s %s' % (addr, state, name))
    if proven:
        nv = len([x for x in matched if x in funcs]) + len([x for x in proven if x in funcs])
        print('%d more proven equivalent (EQUIV); verified total %d/%d (%.1f%% of bytes)' % (
            len(proven), nv, len(funcs), 100.0 * (mb + pb) / total_b))
    if a.target == 'd3d' and not a.files and not a.nonmatching:
        # every D3D game function is verified: one that drops out of both the default build and
        # the #ifdef EQUIV blocks (e.g. a changed guard) must fail the gate, not just lower the count
        lost = sorted(x for x in funcs if x not in matched and x not in proven)
        for x in lost:
            print('unverified: %06x %s' % (x, ck.syms.by_addr.get(x, 'func_%06x' % x)))
        if lost:
            bad.append('%d D3D game functions are neither matched nor proven equivalent' % len(lost))
    if a.target != 'd3d' and not a.files and not a.nonmatching:
        # the other targets are not complete yet: config/<target>/verified.txt lists every function
        # a full run verified before, so a header change that loses one (heap noise) fails the gate
        # instead of silently lowering the count. --update-baseline records a new, larger set.
        base = os.path.join(REPO, 'config', a.target, 'verified.txt')
        now = {x for x in funcs if x in matched or x in proven}
        before = set()
        if os.path.exists(base):
            before = {int(l.split()[0], 16) for l in open(base) if l.strip() and not l.startswith('#')}
        lost = sorted(before - now)
        for x in lost:
            print('unverified: %06x %s (verified in %s)' % (x, ck.syms.by_addr.get(x, 'func_%06x' % x),
                                                              os.path.relpath(base, REPO)))
        if lost:
            bad.append('%d %s functions verified before are neither matched nor proven equivalent' % (len(lost), a.target))
        elif a.update_baseline:
            with open(base, 'w') as f:
                f.write('# functions a full progress.py --target %s run verified; the gate fails if one is lost\n' % a.target)
                for x in sorted(now):
                    f.write('%06x %s\n' % (x, ck.syms.by_addr.get(x, 'func_%06x' % x)))
            print('baseline: %d verified functions written to %s' % (len(now), os.path.relpath(base, REPO)))
    if a.nonmatching:
        print('%d non-matching drafts' % len(drafts))
    print('FAIL' if bad else 'OK')
    sys.exit(1 if bad else 0)


def target_progress(a):
    """Check the app-labeled functions in a standalone PE target."""
    target = a.target
    _, fp = S.target_paths(target)
    funcs = {r[0]: r[1] for r in S.load_func_rows(fp) if len(r) > 3 and r[3] == 'app'}
    files = a.files or sorted(glob.glob(os.path.join(REPO, 'src', target, '**', '*.c'), recursive=True))
    files = [os.path.abspath(f) for f in files]
    msvc.ensure()
    ck = Checker(target=target)
    bad, matched = [], {}
    for src in files:
        obj = os.path.join(REPO, 'build', target, os.path.relpath(src, os.path.join(REPO, 'src', target))[:-2] + '.obj')
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        ok, out = msvc.compile(src, obj, S.flags_for(src) + (['/DNON_MATCHING'] if a.nonmatching else []),
                               [os.path.join(REPO, 'include'), os.path.dirname(src)])
        if not ok:
            bad.append('%s: compile failed %s' % (os.path.relpath(src, REPO), out))
            continue
        res, _, base = ck.check(Obj(open(obj, 'rb').read(), obj))
        lits = dict(ck.lits)
        diffs = [r[0] for r in res if r[2] == 'DIFF' and r[1] in funcs]
        ent = {}
        if diffs and not a.nonmatching:
            # compiler heap noise, as for D3D's MATCH*
            ent = entropy_search(ck, src, S.flags_for(src), [os.path.join(REPO, 'include'), os.path.dirname(src)], diffs)
            ck.lits = lits
        for name, addr, status, info in res:
            if addr not in funcs:
                bad.append('%s: %s at %08x is not labeled app' % (os.path.relpath(src, REPO), name, addr))
            elif status == 'MATCH' or name in ent:
                matched[addr] = src
            elif a.nonmatching:
                continue
            else:
                bad.append('%s: %s at %08x does not match' % (os.path.relpath(src, REPO), name, addr))
        for sec, address, size, diffs in ck.data_check(Obj(open(obj, 'rb').read(), obj), base):
            if diffs and not a.nonmatching:
                bad.append('%s: %s data at %08x differs in %d of %d bytes' % (
                    os.path.relpath(src, REPO), sec, address, len(diffs), size))

    # Keep standalone targets under the same EQUIV gate as D3D. The target
    # checker supplies its own executable, symbol map, and address table.
    proven, noisy, exact_in_equiv, oa_proofs = {}, [], [], []
    if not a.nonmatching:
        import equiv
        from equivcheck import void_functions
        equiv.configure(ck.exe, ck.funcs)
        eq_files = [f for f in files if equiv_block_functions(f)]
        with cf.ThreadPoolExecutor(a.j) as ex:
            eq_results = list(ex.map(build_equiv, eq_files))
        for src, ok, out, obj in eq_results:
            rel = os.path.relpath(src, REPO)
            if not ok:
                bad.append('%s: EQUIV build failed\n%s' % (rel, out))
                continue
            voids = void_functions(src)
            in_equiv = equiv_block_functions(src)
            equiv.for_file(src, a.target)
            res, _, _ = ck.check(Obj(open(obj, 'rb').read(), obj))
            failed, initial_diffs, proof_results = {}, {}, {}

            def proof(name, addr, info, quick=False):
                key = addr, info[0], info[1], name not in voids, quick
                if key not in proof_results:
                    proof_results[key] = equiv.equivalent(
                        info[0], info[1], addr, returns_value=name not in voids, quick=quick)
                return proof_results[key]

            for name, addr, status, info in res:
                if name not in in_equiv:
                    continue
                if status == 'NOADDR':
                    bad.append('%s: %s has no address (EQUIV build)' % (rel, name))
                    continue
                if addr not in funcs:
                    bad.append('%s: %s at %08x is not labeled app (EQUIV build)' % (rel, name, addr))
                    continue
                if addr in matched:
                    continue
                if status == 'MATCH':
                    proven[addr] = rel
                    exact_in_equiv.append('%s: %s' % (rel, name))
                    continue
                good, why = proof(name, addr, info, quick=True)
                if good:
                    proven[addr] = rel
                    if 'assuming no aliasing' in why:
                        oa_proofs.append('%s: %s' % (rel, name))
                else:
                    failed[name] = (addr, why)
                    initial_diffs[name] = info

            if failed:
                def judge(name, addr, status, info):
                    return status == 'MATCH' or (status == 'DIFF' and proof(name, addr, info)[0])

                def quick_judge(name, addr, status, info):
                    return status == 'MATCH' or (status == 'DIFF' and proof(name, addr, info, quick=True)[0])

                def full_initial(name):
                    addr, _ = failed[name]
                    good, why = proof(name, addr, initial_diffs[name])
                    if good:
                        proven[addr] = rel
                        if 'assuming no aliasing' in why:
                            oa_proofs.append('%s: %s' % (rel, name))
                    else:
                        failed[name] = (addr, why)
                    return good

                flags = S.flags_for(src) + ['/DEQUIV', '/DNON_MATCHING']
                includes = [os.path.join(REPO, 'include'), os.path.dirname(src)]
                ent = equiv_entropy_search(ck, src, flags, includes, list(failed),
                                           judge, quick_judge, full_initial)
                for name, (addr, why) in sorted(failed.items()):
                    if addr in proven:
                        continue
                    if name in ent:
                        proven[addr] = rel
                        noisy.append('%s: %s (EQUIV, k=%d)' % (rel, name, ent[name]))
                    else:
                        bad.append('%s: %s %08x in the EQUIV build is neither exact nor equivalent: %s' % (
                            rel, name, addr, why))

    verified = set(matched) | set(proven)
    missing = sorted(set(funcs) - verified)
    print('%d/%d %s app functions match, %d more proven equivalent; verified %d/%d' % (
        len(matched), len(funcs), target, len(proven), len(verified & set(funcs)), len(funcs)))
    if noisy:
        print('%d functions match only with dummy declarations in front (compiler heap noise):' % len(noisy))
        for line in noisy:
            print('  ' + line)
    if oa_proofs:
        print('%d EQUIV proofs rely on the files\' /Oa or /Ow contract:' % len(oa_proofs))
        for line in oa_proofs:
            print('  ' + line)
    if exact_in_equiv:
        print('%d functions in #ifdef EQUIV blocks match byte for byte in that build:' % len(exact_in_equiv))
        for line in exact_in_equiv:
            print('  ' + line)
    if missing:
        print('%d app functions remain unverified: %s' % (len(missing), ' '.join('%08x' % x for x in missing)))
    for line in bad:
        print(line)
    print('FAIL' if bad else 'OK')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
