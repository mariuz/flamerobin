{%confgui%}set term !! ;

create trigger CT_{%object_name%} for {%object_name%}
active after update position 99
as
begin
    if ({%foreach:column:
        or :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:old.{%object_name%} is distinct from new.{%object_name%}%}%})
    then
    begin
        /* do something */
    end
end!!

set term ; !!
