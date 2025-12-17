thisDir   = fileparts(mfilename('fullpath'));           % dossier de test2.m
dataDir   = fullfile(thisDir, '..', 'data_input');      % dossier data_input
csvPath   = fullfile(dataDir, 'Bulk_density2_modif.csv');
T = readtable(csvPath);    % lit le CSV dans une table
%nomsColonnes = T.Properties.VariableNames  me donne