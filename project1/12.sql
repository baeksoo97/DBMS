select distinct P.name, P.type
from Pokemon P
where P.id in (
  select C.pid
  from CatchedPokemon C
  where C.level >= 30
)
order by P.name;
