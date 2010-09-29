{%confgui%}update
  {%object_name%}
set {%foreach:column:, :{%ifcontains:{%getconf:{%object_path%}/columnNames%}:{%object_name%}:
  {%object_name%} = '{%object_name%} {%column_nulloption%}'%}%}
where
  {%primary_key:{%constraint_columns: and
  : = '?'%}%};
