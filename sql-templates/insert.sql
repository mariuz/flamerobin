{%confgui%}insert into {%object_name%} ({%getconf:{%object_path%}/columnNames%})
values ({%foreach:column:, :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:
    {%object_name%} {%column_nulloption%}%}%}
);
