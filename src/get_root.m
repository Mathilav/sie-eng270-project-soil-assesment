function [projroot, here] = get_root()
% Get project root directory, which should be one directory above this one.
% The current directory is also returned as the 2nd value.
here = fileparts(mfilename('fullpath'));
projroot = fileparts(here);
end