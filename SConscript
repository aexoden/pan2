Import('env')
libs=[]
SConscript(r'uulib/SConscript',exports=['env','libs'])
SConscript(r'pan/SConscript',exports=['env','libs'])

c=env.Command('pan.iss','pan.iss.in',Copy('$TARGET','$SOURCE'))
env.Default(c)
