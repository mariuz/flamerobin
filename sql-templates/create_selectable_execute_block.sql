{%confgui%}set term !! ;

execute block
returns ( {%foreach:column:, :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:
    {%object_name%} {%columninfo:datatype%}%}%}
)
as
begin
    for select {%foreach:column:, :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:
        a.{%object_name%}%}%}
    from {%object_name%} a
    into {%foreach:column:, :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:
        {%object_name%}%}%} 
    do
    begin
        suspend;
    end
end!!

set term ; !!
