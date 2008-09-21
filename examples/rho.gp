rho1(n)=
{
  local(x,y);

  x=2; y=5;
  while(gcd(y-x,n)==1,
    x=(x^2+1)%n;
    y=(y^2+1)%n; y=(y^2+1)%n
  );
  gcd(n,y-x)
}

rho2(n)=
{
  local(m);

  m=rho1(n);
  if (isprime(m),
    print(m)
  ,
    rho2(m)
  );
  if (isprime(n/m),
    print(n/m)
  ,
    rho2(n/m)
  );
}

rho(n)=
{
  local(m);

  m=factor(n,0); print(m);
  n=m[length(m[,1]),1];
  if (!isprime(n),rho2(n));
}

rhobrent(n)=
{
  x=y=x1=2; k=l=p=1; c=0;
  while (1,
    x=(x^2+1)%n; p=(p*(x1-x))%n; c++;
    if (c==20,
      if (gcd(p,n)>1, break);
      y=x;c=0
    );
    k--;
    if (!k,
      if (gcd(p,n)>1, break);

      x1=x; k=l; l <<= 1; \\ l = 2*l
      for (j=1,k, x=(x^2+1)%n);
      y=x; c=0
    )
  );
  g=1;
  while (g==1,
    y=(y^2+1)%n;
    g=gcd(x1-y,n)
  );
  if (g==n,error("algorithm fails"));
  g
}
