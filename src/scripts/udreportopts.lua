-------------------------------------------------------------------------------
-- report options
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- I. Output Formats
-------------------------------------------------------------------------------
produce_html = 1

-------------------------------------------------------------------------------
-- II. HTML specific options
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
-- Generally ASCII renders better in Internet Explorer and 
-- UTF-16 renders better in FireFox. UTF-16 is of course required to
-- properly render Asian characters such as Japanese and Chinese.
-------------------------------------------------------------------------------
use_utf16 = 0

-------------------------------------------------------------------------------
-- Set enable_sorting to zero if your web browser is too old
-- and you have error messages about ivalid javascript code.
-------------------------------------------------------------------------------
enable_sorting = 1

-------------------------------------------------------------------------------
-- Set this parameter to 1 if you prefer to look at filenames splitted
-- into few short lines. If you prefer to use fullscreen mode of your
-- web browser then set this parameter to zero.
-------------------------------------------------------------------------------
split_long_names = 1
-------------------------------------------------------------------------------
-- Set here maximum number of characters per line in filename cells.
-------------------------------------------------------------------------------
max_chars_per_line = 50

-------------------------------------------------------------------------------
-- This stylesheet is used to set styles for various report elements.
-------------------------------------------------------------------------------
style = [[
<style>
td {font-family: monospace; font-size: 10pt}
.c {text-align: center}
.f {background-color: #000000; color: #FFFFFF}
</style>
]]

title_tags = {open = "<pre><h3>", close = "</h3></pre>"}

-------------------------------------------------------------------------------
-- Main report table properties.
-------------------------------------------------------------------------------
table_style = [[border="1" color="#FFAA55" cellspacing="0" width="100%"]]
