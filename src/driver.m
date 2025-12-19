% DRIVER: orchestrate the pipeline using explicit input/output arguments

projroot = get_root();

% Ensure results directory exists (all generated files must go here)
resultsDir = fullfile(projroot, 'results');
if ~exist(resultsDir, 'dir')
    mkdir(resultsDir);
end

% Source folders
src_wc   = fullfile(projroot, 'src', 'water_content');
src_poro = fullfile(projroot, 'src', 'porosity');
src_plot = fullfile(projroot, 'src', 'plot');

% Input / output paths used by the pipeline
in_temp       = fullfile(projroot, 'data', 'temp_prec.csv');
in_ray        = fullfile(projroot, 'data', 'rayonnement.csv');
out_temp_ray  = fullfile(resultsDir, 'temp_prec_ray.csv');
% The C model is expected (per instructions) to consume "results/Temp_prec_ray.csv"
out_temp_ray_c = fullfile(resultsDir, 'Temp_prec_ray.csv');

theta_out     = fullfile(resultsDir, 'theta_output.csv');

in_travail    = fullfile(projroot, 'data', 'travail_agricole.csv');
in_itk        = fullfile(projroot, 'data', 'ITK.csv');
out_poro_full = fullfile(resultsDir, 'data_porosity_full.csv');
out_poro_full_out = fullfile(resultsDir, 'data_porosity_full_out.csv');

mesure2       = fullfile(projroot, 'data', 'mesures_porosity_2.csv');
mesure1       = fullfile(projroot, 'data', 'mesures_porosity_1.csv');
poro_plot_png  = fullfile(resultsDir, 'porosity_plot.png');
theta_plot_png = fullfile(resultsDir, 'theta_obsVScomp_plot.png');

cwd = pwd();
try
    %% 1) Run MATLAB preprocessing: data_water_content
    if exist(src_wc, 'dir') == 7
        addpath(src_wc);
    else
        error('Missing folder: %s', src_wc);
    end

    fprintf('Running data_water_content with inputs and writing to %s\n', out_temp_ray);
    if exist('data_water_content','file') == 2 || exist('data_water_content','file') == 6
        try
            % function now accepts (meteo_file, ray_file, out_csv)
            data_water_content(in_temp, in_ray, out_temp_ray);
        catch ME
            warning('data_water_content failed: %s', ME.message);
        end
    else
        warning('data_water_content not found in %s', src_wc);
    end

    % Ensure the C-model-compatible name exists (Windows is case-insensitive but keep both names)
    try
        copyfile(out_temp_ray, out_temp_ray_c);
    catch
        % ignore if copy fails; the subsequent C run will error if file missing
    end

    %% 2) Compile and run modele_water_content.c with argv: input output
    c_src_wc = fullfile(src_wc, 'modele_water_content.c');
    if exist(c_src_wc,'file')
        % ensure a central bin folder for compiled executables
        bin_dir = fullfile(projroot, 'bin');
        if ~exist(bin_dir, 'dir'); mkdir(bin_dir); end

        exe_name = 'modele_water_content';
        if ispc; exe_name = [exe_name '.exe']; end
        exe_path = fullfile(bin_dir, exe_name);
        if ~exist(exe_path,'file')
            gcc_cmd = sprintf('gcc -O2 -o "%s" "%s" -lm', exe_path, c_src_wc);
            fprintf('Compiling %s\n', c_src_wc);
            [st,out] = system(gcc_cmd);
            if st ~= 0
                warning('Compilation failed: %s', out);
            end
        end

        if exist(out_temp_ray_c,'file')
            run_cmd = sprintf('"%s" "%s" "%s"', exe_path, out_temp_ray_c, theta_out);
            fprintf('Running modele_water_content: %s\n', run_cmd);
            [st,out] = system(run_cmd);
            if st ~= 0
                warning('modele_water_content failed: %s', out);
            end
        else
            warning('Expected input for C model not found: %s', out_temp_ray_c);
        end
    else
        warning('C source not found: %s', c_src_wc);
    end

    %% 3) Run extraction_porosity MATLAB script to produce data_porosity_full.csv
    if exist(src_poro,'dir') == 7
        addpath(src_poro);
    else
        error('Missing folder: %s', src_poro);
    end

    fprintf('Running extraction_porosity to create %s\n', out_poro_full);
    if exist('extraction_porosity','file') == 2 || exist('extraction_porosity','file') == 6
        try
            extraction_porosity(in_travail, in_itk, theta_out, out_poro_full);
        catch ME
            warning('extraction_porosity failed: %s', ME.message);
        end
    else
        warning('extraction_porosity.m not found in %s', src_poro);
    end

    %% 4) Compile and run modele_porosity.c with argv: input output
    c_src_poro = fullfile(src_poro, 'modele_porosity.c');
    if exist(c_src_poro,'file')
        % ensure bin folder exists (created earlier for water model)
        bin_dir = fullfile(projroot, 'bin');
        if ~exist(bin_dir, 'dir'); mkdir(bin_dir); end

        exe_poro = 'modele_porosity';
        if ispc; exe_poro = [exe_poro '.exe']; end
        exe_poro_path = fullfile(bin_dir, exe_poro);
        if ~exist(exe_poro_path,'file')
            gcc_cmd = sprintf('gcc -O2 -o "%s" "%s" -lm', exe_poro_path, c_src_poro);
            fprintf('Compiling %s\n', c_src_poro);
            [st,out] = system(gcc_cmd);
            if st ~= 0
                warning('Compilation failed: %s', out);
            end
        end

        if exist(out_poro_full,'file')
            run_cmd = sprintf('"%s" "%s" "%s"', exe_poro_path, out_poro_full, out_poro_full_out);
            fprintf('Running modele_porosity: %s\n', run_cmd);
            [st,out] = system(run_cmd);
            if st ~= 0
                warning('modele_porosity failed: %s', out);
            end
        else
            warning('Expected porosity input not found: %s', out_poro_full);
        end
    else
        warning('C source not found: %s', c_src_poro);
    end

    %% 5) Plot porosity results
    if exist(src_plot,'dir') == 7
        addpath(src_plot);
    else
        error('Missing folder: %s', src_plot);
    end

    fprintf('Calling plot_poro with porosity result and measurements\n');
    if exist('plot_poro','file') == 2 || exist('plot_poro','file') == 6
        try
            plot_poro(out_poro_full_out, mesure2, mesure1, poro_plot_png);
        catch ME
            warning('plot_poro failed: %s', ME.message);
        end
    else
        warning('plot_poro not found in %s', src_plot);
    end

    %% 6) Plot theta comparison
    fprintf('Calling plot_theta with computed and observed theta\n');
    if exist('plot_theta','file') == 2 || exist('plot_theta','file') == 6
        try
            plot_theta(theta_out, fullfile(projroot,'data','real_SWC.csv'), theta_plot_png);
        catch ME
            warning('plot_theta failed: %s', ME.message);
        end
    else
        warning('plot_theta not found in %s', src_plot);
    end

    fprintf('Driver finished. All outputs should be in %s\n', resultsDir);
catch ME
    cd(cwd);
    rethrow(ME);
end