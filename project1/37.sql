select T2.name, sum(C2.level)
from Trainer T2, CatchedPokemon C2
where T2.id = C2.owner_id
group by T2.id
having sum(C2.level) = (
  select sum(C.level)
  from Trainer T, CatchedPokemon C
  where T.id = C.owner_id
  group by T.id
  order by sum(C.level) desc limit 1
)
order by T2.name;
