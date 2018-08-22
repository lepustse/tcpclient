from building import * 

# get current dir path
cwd = GetCurrentDir()

# init src and inc vars
src = []
inc = []

# add tcpclient common include
inc = inc + [cwd + '/inc']

# add tcpclient basic code
src = src + [cwd + '/src/tcpclient.c']

# add tcpclient test code
src = src + [cwd + '/examples/tcpclient_example.c']


# add group to IDE project
group = DefineGroup('tcpclient', src, depend = [''], CPPPATH = inc)

Return('group')
