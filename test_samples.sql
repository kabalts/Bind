create database testdb
use testdb
use nonexist
create table person (id int primary, name string)
insert into person values (1, 'Alice')
insert into person values (2, 'Bob')
insert into person values (3, 'Charlie')
insert into person values (1, 'DuplicatePK')
insert into person values (3, 'AlsoDup')
select * from person
select * from person where id = 2
select id, name from person where id > 1
update person set name = 'Alice2' where id = 1
update person set name = 'Nope' where id = 999
select * from person where id = 1
delete from person where id = 3
delete from person where id = 999
select * from person
delete from person where id = 1
insert into person values (1, 'AliceReinserted')
select * from person
drop table person
drop database testdb
exit