select distinct A.name
from 
(
  select distinct P.name as name
  from Trainer T, CatchedPokemon C, Pokemon P
  where T.id = C.owner_id
  and P.id = C.pid
  and T.hometown = "Sangnok City"
) as A,
(
  select distinct P.name as name
  from Trainer T, CatchedPokemon C, Pokemon P
  where T.id = C.owner_id
  and P.id = C.pid
  and T.hometown = "Brown City"
) as B
where A.name = B.name
order by A.name;


