select P.id, P.name, P2.name, P3.name
from Evolution as E
join Pokemon as P
on P.id = E.before_id
join Evolution as E2
on E2.before_id = E.after_id
join Pokemon as P2
on P2.id = E2.before_id
join Pokemon as P3
on P3.id = E2.after_id
order by P.id;
