select T.name
from Trainer T , CatchedPokemon C
where T.id = C.owner_id
group by T.id
having count(T.id) >= 3
order by count(T.id) desc;