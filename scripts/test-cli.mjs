import assert from 'node:assert/strict'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import {spawnSync} from 'node:child_process'
import {test, after} from 'node:test'
import {fileURLToPath} from 'node:url'

const root = fileURLToPath(new URL('../', import.meta.url))
const compiler = path.resolve(process.env.LOTUS_COMPILER ?? path.join(root, 'cmake-build-debug/lotus'))
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'lotus-cli-'))
const suffix = process.platform === 'win32' ? '.exe' : '.out'
const source = path.join(temp, 'hello world.lt')
fs.writeFileSync(source, 'fn twice(n: int): int { return n * 2 }\nprint(twice(21))\n')
after(() => fs.rmSync(temp, {recursive: true, force: true}))
const run = (args, env = {}) => spawnSync(compiler, args, {
    cwd: temp, encoding: 'utf8', env: {...process.env, LOTUS_LIBS: path.join(root, 'libs'), ...env}
})

const expectOutput = result => {
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout.replaceAll('\r\n', '\n'), '42\n')
    assert.equal(result.stderr, '')
}

for (const args of [[source], ['run', source]])
    test(`execute ${args[0] === 'run' ? 'explicit' : 'implicit'} run`, () => {
        const result = run(args)
        expectOutput(result)
        assert.equal(fs.existsSync(path.join(temp, 'output.ll')), false)
    })

test('check does not execute the source', () => {
    const result = run(['check', source])
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout, '')
})

test('help needs no source or runtime', () => {
    const result = run(['--help'], {LOTUS_LIBS: '/missing-runtime'})
    assert.equal(result.status, 0)
    assert.match(result.stdout, /Usage: lotus/)
})

test('new creates a runnable project from the bundled template without loading runtime', () => {
    const name = 'привет world'
    const result = run(['new', name, temp], {LOTUS_LIBS: '/missing-runtime'})
    assert.equal(result.status, 0, result.stderr)
    const file = path.join(temp, name, 'main.lt')
    assert.equal(fs.readFileSync(file, 'utf8'), fs.readFileSync(path.join(root, 'templates/main.lt'), 'utf8'))
    const execution = run(['run', file])
    assert.equal(execution.status, 0, execution.stderr)
    assert.equal(execution.stdout.replaceAll('\r\n', '\n'), 'Hello, World!\n')
})

test('new preserves an existing directory and its files', () => {
    const directory = path.join(temp, 'existing')
    fs.mkdirSync(directory)
    fs.writeFileSync(path.join(directory, 'main.lt'), 'keep me')
    const result = run(['new', 'existing', '.'])
    assert.equal(result.status, 1)
    assert.match(result.stderr, /Cannot create project/)
    assert.equal(fs.readFileSync(path.join(directory, 'main.lt'), 'utf8'), 'keep me')
})

test('new reports invalid arguments and missing parent directories', () => {
    for (const args of [[], ['name'], ['name', '.', 'extra']]) {
        const result = run(['new', ...args])
        assert.equal(result.status, 1)
        assert.match(result.stderr, /Usage: lotus new NAME PATH/)
    }
    for (const name of ['', '.', '..', '../escape', 'nested/name', 'nested\\name']) {
        const result = run(['new', name, '.'])
        assert.equal(result.status, 1)
        assert.match(result.stderr, /Expected a project name/)
    }
    assert.match(run(['new', 'name', 'missing-parent']).stderr, /Cannot create project/)
    assert.match(run(['new', 'name', '']).stderr, /Expected a parent directory/)
    assert.equal(run(['new', '--help']).status, 0)
})

for (const [args, message] of [
    [['--unknown'], 'Unknown option'],
    [['--libs'], 'Missing value'],
    [[source, source], 'Expected one source'],
    [['missing.lt'], 'Cannot read'],
    [['--libs', 'missing', source], 'Cannot open library directory'],
    [['check', '--emit-ir', 'out.ll', source], '--emit-ir requires run or build'],
    [['--emit-ir', 'missing/out.ll', source], 'Cannot write IR']
]) test(message, () => {
    const result = run(args)
    assert.equal(result.status, 1)
    assert.equal(result.stdout, '')
    assert.ok(result.stderr.includes(message), result.stderr)
})

test('debug and IR output are explicit', () => {
    const output = path.join(temp, 'saved.ll')
    const result = run(['run', '--debug', '--emit-ir', output, source])
    assert.equal(result.status, 0, result.stderr)
    assert.match(result.stdout, /###### RUNNING main\(\):\r?\n42\r?\n/)
    assert.match(fs.readFileSync(output, 'utf8'), /define[^\n]* i32 @main/)
})

test('explicit library directory overrides environment', () => {
    const result = run(['--libs', path.join(root, 'libs'), source], {LOTUS_LIBS: '/missing-runtime'})
    assert.equal(result.status, 0, result.stderr)
})

test('JIT does not create an executable', () => {
    const file = path.join(temp, 'jit.lt')
    fs.copyFileSync(source, file)
    const result = run(['run', file])
    expectOutput(result)
    assert.equal(fs.existsSync(`${file}${suffix}`), false)
})

test('build creates an executable without running it', () => {
    const result = run(['build', source])
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout, '')
    const executable = spawnSync(`${source}${suffix}`, {cwd: os.tmpdir(), encoding: 'utf8'})
    expectOutput(executable)
})

test('linker failure does not execute an old binary', () => {
    const result = run(['build', source], {CC: '/missing-cc'})
    assert.equal(result.status, 1)
    assert.equal(result.stdout, '')
    assert.match(result.stderr, /Cannot execute/)
})

test('diagnostics include the input path', () => {
    const file = path.join(temp, 'invalid.lt')
    fs.writeFileSync(file, 'print(unknown)')
    const result = run(['check', file])
    assert.equal(result.status, 1)
    assert.ok(result.stderr.includes(`${file}:1:`), result.stderr)
})

test('reject embedded null bytes', () => {
    const file = path.join(temp, 'null.lt')
    fs.writeFileSync(file, 'print(1)\0print(2)')
    assert.match(run([file]).stderr, /contains a null byte/)
})

test('default runtime works outside the checkout', () => {
    const result = run([source], {LOTUS_LIBS: undefined})
    expectOutput(result)
})

test('missing source prints usage and fails', () => {
    const result = run([])
    assert.equal(result.status, 1)
    assert.match(result.stdout, /Usage: lotus/)
})

test('run does not need a linker', () => {
    expectOutput(run(['run', source], {CC: '/missing-cc'}))
})

test('build exports IR without executing', () => {
    const output = path.join(temp, 'build.ll')
    const result = run(['build', '--emit-ir', output, source])
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout, '')
    assert.match(fs.readFileSync(output, 'utf8'), /define[^\n]* i32 @main/)
})

test('native arguments keep their values across the platform ABI', () => {
    const file = path.join(temp, 'native values.lt')
    fs.writeFileSync(file, `
print(1, "|", 2l, "|", 3.5f, "|", 4.5, "|", true, "|", "hello")
print(toBool(1), toBool(0), toBool("true"), toBool(1.5))
long big = 9223372036854775807l
print(big)
double exponent = (double)toInt("3")
print(2.0 ** exponent)
string value = "a"
for i in 1..100 value += "b"
print(value == "a" + value)
`)
    const expected = '1|2|3.500000|4.500000|true|hello\ntruefalsetruetrue\n9223372036854775807\n8.000000\nfalse\n'
    const jit = run(['run', file])
    assert.equal(jit.status, 0, jit.stderr)
    assert.equal(jit.stdout.replaceAll('\r\n', '\n'), expected)
    const build = run(['build', file])
    assert.equal(build.status, 0, build.stderr)
    const executable = spawnSync(`${file}${suffix}`, {cwd: os.tmpdir(), encoding: 'utf8'})
    assert.equal(executable.status, 0, executable.stderr)
    assert.equal(executable.stdout.replaceAll('\r\n', '\n'), expected)
})


test('UTF-8 paths and runtime strings work with redirected input', () => {
    const file = path.join(temp, 'пример.lt')
    fs.writeFileSync(file, 'print(input() + "世界")\n')
    const options = {cwd: temp, encoding: 'utf8', input: 'привет\n', env: {...process.env, LOTUS_LIBS: path.join(root, 'libs')}}
    const jit = spawnSync(compiler, ['run', file], options)
    assert.equal(jit.status, 0, jit.stderr)
    assert.equal(jit.stdout.replaceAll('\r\n', '\n'), 'привет世界\n')
    const build = run(['build', file])
    assert.equal(build.status, 0, build.stderr)
    const executable = spawnSync(`${file}${suffix}`, [], options)
    assert.equal(executable.status, 0, executable.stderr)
    assert.equal(executable.stdout.replaceAll('\r\n', '\n'), 'привет世界\n')
})

test('arrays work in JIT and executable builds', () => {
    const file = path.join(temp, 'arrays.lt')
    fs.writeFileSync(file, `
fn words(): string[] { return ["hel" + "lo", "world"] }
string[] items = words()
string[] alias = items
items = []
alias[1] = "Lotus"
for word in alias print(word)
int[] values = new int[2]
values[1] = 42
print(values[1])
print(items.length)
`)
    const expected = 'hello\nLotus\n42\n0\n'
    const jit = run(['run', file])
    assert.equal(jit.status, 0, jit.stderr)
    assert.equal(jit.stdout.replaceAll('\r\n', '\n'), expected)
    const build = run(['build', file])
    assert.equal(build.status, 0, build.stderr)
    const executable = spawnSync(`${file}${suffix}`, {cwd: temp, encoding: 'utf8'})
    assert.equal(executable.status, 0, executable.stderr)
    assert.equal(executable.stdout.replaceAll('\r\n', '\n'), expected)
})

test('array bounds fail at runtime in both execution modes', () => {
    const file = path.join(temp, 'bounds.lt')
    fs.writeFileSync(file, 'int[] values = [1]\nprint(values[values.length])\n')
    assert.equal(run(['check', file]).status, 0)
    const jit = run(['run', file])
    assert.equal(jit.status, 1)
    assert.match(jit.stderr, /Array index 1 out of bounds for length 1/)
    assert.equal(run(['build', file]).status, 0)
    const executable = spawnSync(`${file}${suffix}`, {cwd: temp, encoding: 'utf8'})
    assert.equal(executable.status, 1)
    assert.match(executable.stderr, /Array index 1 out of bounds for length 1/)
})
