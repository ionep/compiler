#!/usr/bin/env python3
import subprocess
from pathlib import Path
import signal

def run(cmd, cwd=None):
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = proc.communicate()
    rc = proc.returncode
    if rc < 0:
        sig = -rc
        name = signal.Signals(sig).name  # e.g. 'SIGSEGV'
        raise RuntimeError(f"Process {cmd[0]} crashed on {name} ({sig})\n{err or out}")
    return rc, out.strip(), err.strip()

def load_groundtruth(gt_path):
    gt = {}
    for line in gt_path.read_text().splitlines():
        parts = line.split()
        if len(parts) >= 3:
            key = (parts[0], parts[1])
            gt[key] = parts[2]
    return gt

def main():
    root        = Path(__file__).parent.resolve()
    regex_dir   = root / "tests" / "regex"
    strings_dir = root / "tests" / "strings"
    gt_path     = root / "tests" / "groundtruth.txt"
    results     = root / "tests" / "test_results.txt"
    comp        = root / "tests" / "comparison.txt"

    # Load expected answers
    if not gt_path.exists():
        print(f"ERROR: groundtruth.txt not found at {gt_path}")
        return
    groundtruth = load_groundtruth(gt_path)

    # fresh start
    for p in (results, comp):
        if p.exists(): p.unlink()

    total = passed = failed = 0

    with results.open("w") as fout, comp.open("w") as cmpf:
        for rx in sorted(regex_dir.glob("*.txt")):
            base   = rx.stem
            # binary = root / f"rexec_{base}"
            binary = root / f"rexec"

            # 1) generate
            code, out, err = run([str(root/"generate"), str(rx)])
            if code != 0:
                actual = f"GENERATE_ERROR"
                # no string tests in this case; record as failure for each
                # but here we just log the generate error line
                fout.write(f"{rx.name} -- {actual} -- {err or out}\n")
                total += 1
                exp = groundtruth.get((rx.name, ""), "N/A")
                cmpf.write(f"{rx.name} <no-string> {exp} {actual} FAIL\n")
                failed += 1
                continue

            # 2) compile
            code, out, err = run(["gcc", str(regex_dir/"rexec.c"), "-o", str(regex_dir/str(binary))])
            if code != 0:
                actual = "COMPILE_ERROR"
                fout.write(f"{rx.name} -- {actual} -- {err or out}\n")
                total += 1
                exp = groundtruth.get((rx.name, ""), "N/A")
                cmpf.write(f"{rx.name} <no-string> {exp} {actual} FAIL\n")
                failed += 1
                continue

            # 3) run string tests
            for st in sorted(strings_dir.glob(f"{base}_*.txt")):
                code, out, err = run([str(binary), str(st)])
                if code == 0:
                    actual = out or "<no output>"
                else:
                    actual = f"RUNTIME_ERROR"

                # write actual results
                fout.write(f"{rx.name} {st.name} {actual}\n")

                # compare
                exp = groundtruth.get((rx.name, st.name), None)
                status = "PASS" if exp == actual else "FAIL"

                cmpf.write(f"{rx.name} {st.name} {exp or 'MISSING'} {actual} {status}\n")

                total += 1
                if status == "PASS":
                    passed += 1
                else:
                    failed += 1

    # summary
    print(f"Done.\nResults: {results}\nComparison: {comp}")
    print(f"Total: {total}, Passed: {passed}, Failed: {failed}")

if __name__ == "__main__":
    main()
