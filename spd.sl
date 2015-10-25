surface
spd (float Ka = 1;
     float Kd = .5;
     float Ks = .5;
     float roughness = .1;
     color specularcolor = 1;
     float reflected = 0.;
     float transmitted = 0.;
     float index = 1.0;)
{
  point Nf;
  color r = 0, tr = 0;
  point Rfldir, Rfrdir;
  point IN;
     
  Nf = faceforward (normalize(N),I);
     
  if (reflected > .001 || transmitted > .001) {
      /* Construct a normalized incident vector */ 
      IN = normalize (I);
     
      if (reflected > .001) {
   Rfldir = normalize(reflect(IN, Nf)); 
   r = reflected * trace (P, Rfldir);
      }
      if (transmitted > .001) {
   Rfrdir = normalize(refract(IN, Nf, index)); 
   tr = transmitted * trace (P, Rfrdir);
      }
  }
     
  Oi = Os;
  Ci = Os * ( Cs * (Ka*ambient() + Kd*diffuse(Nf) + r + tr) +
       specularcolor * Ks*specular(Nf,-normalize(I),roughness));
}
