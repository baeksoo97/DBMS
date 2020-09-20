select P.name
from Evolution as E, Pokemon as P
where E.after_id not in(
  select  E2.before_id
  from Evolution as E1, Evolution as E2
  where E2.before_id = E1.after_id)
and E.after_id = P.id
order by P.name;
