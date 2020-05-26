import random

print("Nombre passagers:")
n=int(input());
for i in range(n):
    s1 = random.randint(0,7)
    s2 = s1
    while s2 == s1:
        s2 = random.randint(0,7)
    trans = (s1<5 and s2>=5) or (s1>=5 and s2<5)
    quart = int(n/4)
    t_max = quart + random.randint(0,2*quart)
    t = random.randint(0,t_max-quart)
    print('# {} {} {} {} {} {}'.format(i,s1,s2,t,int(trans),t_max))

