% clear;
close all;
addpath('libsdca-debug');
rng(0);



if 1
  d = 10;
  n = 10;

%   opts.prox = 'entropy';
%   opts.prox = 'knapsack';
  opts.prox = 'lambert_w_exp';
  opts.rhs = 100;
  opts.hi = 10;

%   A = 1000*randn(d,n);
  A = -10:0.01:10;
  B = libsdca_prox(A, opts);
  plot(B)
%   disp(sum(B));
  
%   [X,mu,nu] = prox_entropy_cvx(A, opts.hi, opts.rhs);
%   
%   loss = @(X) 0.5*sum(sum((A - X).^2)) - sum(sum(entr(X)));
%   disp(loss(X)-loss(B));
end

if 0
  cd /BS/mlapin-projects1/work/simplex/test
  runtestcases_2
end

if 0
  load('data/sun397-fv.mat');
  Ktrn = Ktrn-1;
  
  ix = 1:5*3;
  Ktrn = Ktrn(ix,ix);
  Ytrn = Ytrn(ix);
  

%   opts.objective = 'l2_entropy';
  opts.objective = 'l2_topk_hinge';
  opts.C = 1;
  opts.k = 1;
  opts.gamma = 0;
  opts.epsilon = 1e-5;
  opts.check_on_start = 0;
  opts.check_epoch = 1;
  opts.max_epoch = 10;
  opts.summation = 'standard';
  opts.precision = 'double';
  opts.log_level = 'debug';
  opts.log_format = 'long_e';
  opts.is_dual = 1;

  if opts.is_dual
    if ~exist('Ktrn', 'var')
      Ktrn = Xtrn'*Xtrn;
    end
    model = libsdca_solve(Ktrn, Ytrn, opts);
    disp(model);
    [~,pred] = max(model.A*Ktrn);
    fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  else
    model = libsdca_solve(Xtrn, Ytrn, opts);
    disp(model);
    [~,pred] = max(model.W'*Xtrn);
    fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  end
  
  if 0
    opts2 = model;
    opts2.gamma = 0;
    opts2.check_on_start = true;
    if opts2.is_dual
      model2 = libsdca_solve(Xtrn'*Xtrn, Ytrn, opts2);
      disp(model2);
      [~,pred] = max(model2.A*Xtrn'*Xtrn);
      fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
    else
      model2 = libsdca_solve(Xtrn, Ytrn, opts2);
      disp(model2);
      [~,pred] = max(model2.W'*Xtrn);
      fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
    end
  end
  
  if 0
  opts2 = model;
  opts2.check_on_start = true;
  opts2.k = 10;
  
  opts_prox.proj = 'topk_simplex_biased';
  opts_prox.k = opts2.k;
  opts_prox.rhs = opts2.C;
  opts_prox.rho = 1;
  libsdca_prox(opts2.A, opts_prox);
  
  model1 = libsdca_solve(single(Xtrn), Ytrn, opts2);
  disp(model1);
  [~,pred] = max(model1.W'*Xtrn);
  fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  
  opts3 = model1;
  opts3.summation = 'kahan';
  opts3.precision = 'long double';
  model2 = libsdca_solve(single(Xtrn), Ytrn, opts3);
  disp(model2);
  [~,pred] = max(model2.W'*Xtrn);
  fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  end
end
