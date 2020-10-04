select T.name
from Evolution as E, CatchedPokemon as C, Trainer as T
where E.after_id not in(
  select  E2.before_id
  from Evolution as E1, Evolution as E2
  where E2.before_id = E1.after_id)
and E.after_id = C.pid
and C.owner_id = T.id
order by T.name;
