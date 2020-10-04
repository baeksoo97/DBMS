select owner_id, count(owner_id)
from CatchedPokemon
group by owner_id
having count(owner_id) = (
  select count(C.owner_id)
  from CatchedPokemon as C
  group by C.owner_id
  order by count(C.owner_id) desc limit 1
)
order by owner_id;

