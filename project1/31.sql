select P.type
from Pokemon as P
join Evolution as E
on P.id = E.before_id
group by P.type
having count(P.type) >= 3
order by P.type desc;
