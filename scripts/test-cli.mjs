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
const source = path.join(temp, 'hello world.lt')
fs.writeFileSync(source, 'fn twice(n: int): int { return n * 2 }\nprint(twice(21))\n')
after(() => fs.rmSync(temp, {recursive: true, force: true}))
const run = (args, env = {}) => spawnSync(compiler, args, {
    cwd: temp, encoding: 'utf8', env: {...process.env, LOTUS_LIBS: path.join(root, 'libs'), ...env}
})

const expectOutput = result => {
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout, '42\n')
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

for (const [args, message] of [
    [['--unknown'], 'Unknown option'],
    [['--jit', source], 'Unknown option: --jit'],
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
    assert.match(result.stdout, /###### RUNNING main\(\):\n42\n/)
    assert.match(fs.readFileSync(output, 'utf8'), /define[^\n]* i32 @main/)
})

test('explicit library directory overrides environment', () => {
    const result = run(['--libs', path.join(root, 'libs'), source], {LOTUS_LIBS: '/missing-runtime'})
    assert.equal(result.status, 0, result.stderr)
})

test('separator is not supported', () => {
    assert.match(run(['--', source]).stderr, /Unknown option: --/)
})

test('JIT does not create an executable', () => {
    const file = path.join(temp, 'jit.lt')
    fs.copyFileSync(source, file)
    const result = run(['run', file])
    expectOutput(result)
    assert.equal(fs.existsSync(`${file}.out`), false)
})

test('build creates an executable without running it', () => {
    const result = run(['build', source])
    assert.equal(result.status, 0, result.stderr)
    assert.equal(result.stdout, '')
    const executable = spawnSync(`${source}.out`, {cwd: '/', encoding: 'utf8'})
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
