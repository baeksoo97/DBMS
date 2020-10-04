select name
from Pokemon
where type in (
	select P.type
    from Pokemon as P
    group by P.type
    having count(P.type) <= (
      select cnt from 
      (
        (
          select count(P2.type) as cnt
          from Pokemon as P2
          group by P2.type
          order by count(P2.type) desc limit 1
        ) as tmp
      )     
	) and count(P.type) >= (
      select cnt from
      (
        (
          select count(P2.type) as cnt
          from Pokemon as P2
          group by P2.type
          order by count(P2.type) desc limit 2, 1
        ) as tmp
      )
    )
)
order by name;
