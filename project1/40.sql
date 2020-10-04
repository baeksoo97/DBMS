select T2.hometown, C2.level, C2.nickname
from CatchedPokemon C2, Trainer T2, (
  select T.hometown as ht, max(C.level) as m
  from CatchedPokemon C, Trainer T
  where C.owner_id = T.id
  group by T.hometown
) as A
where C2.owner_id = T2.id
and T2.hometown = A.ht
and C2.level = A.m
order by T2.hometown;

