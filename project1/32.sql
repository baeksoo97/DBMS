select name
from Pokemon
where id not in (
  select P.id
  from CatchedPokemon C, Pokemon P
  where C.pid = P.id
)
order by name;
