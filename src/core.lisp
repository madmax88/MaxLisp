;;; nil :: A -> Boolean
(set 'nil?
     (lambda (a)
       (eq a nil)))

;;; append :: [] -> [] -> []
(set 'append
     (lambda (a b)
       (if (nil? a)
           b
           (if (nil? b)
               a
               (cons (car a) (append (cdr a) b))))))

(set 'cadr
     (lambda (a)
       (car (cdr a))))

(set 'not
     (lambda (p)
       (if p nil t)))

(set 'primitive-map
     (lambda (f args)
       (if (not (nil? args))
           (cons (f (car args)) (primitive-map f (cdr args))))))

(set 'primitive-reduce
     (lambda (f initial-or-acc seq)
       (if (nil? seq)
           initial-or-acc
           (primitive-reduce f (f initial-or-acc (car seq)) (cdr seq)))))

(set 'backquote
     (meta-lambda (expression)
       (list '(lambda (--backquote-expression)
               (if (atom? --backquote-expression)
                   --backquote-expression
                   (if (eq (car --backquote-expression) 'comma)
                       (eval (cadr --backquote-expression))
                       (primitive-reduce
                        (lambda (--backquote-acc --backquote-syntax-datum)
                          (append --backquote-acc
                                  (if (atom? --backquote-syntax-datum)
                                      (list (eval (list 'backquote --backquote-syntax-datum)))
                                      (if (eq (car --backquote-syntax-datum) 'comma)
                                          (list (eval (cadr --backquote-syntax-datum)))
                                          (if (eq (car --backquote-syntax-datum) 'comma-at)
                                              (eval (cadr --backquote-syntax-datum))
                                              (list (eval (list 'backquote --backquote-syntax-datum))))))))
                        nil
                        --backquote-expression))))
             (list 'quote expression))))

;;; WE CAN NOW USE BACKQUOTE

(set 'setq
     (meta-lambda (variable-name variable-expression)
        `(set (quote ,variable-name) ,variable-expression)))

(setq defun
     (meta-lambda (function-name function-arguments . function-body)
       `(setq ,function-name
              (lambda ,function-arguments
                ,@function-body))))

(setq defmacro
     (meta-lambda (macro-name macro-arguments . macro-body)
        `(setq ,macro-name
               (meta-lambda ,macro-arguments
                 ,@macro-body))))

;;; WE CAN NOW USE DEFUN, DEFMACRO, AND SETQ

(defmacro when (--when-expression --when-body)
  `(if ,--when-expression
       ,--when-body))

(defmacro unless (--unless-expression --unless-body)
  `(if (not ,--unless-expression)
       ,--unless-body))

(defmacro let (--let-bindings . body)
  `((lambda ,(primitive-map car --let-bindings)
      ,@body)
    ,@(primitive-map cadr --let-bindings)))

(defmacro progn (--progn-a . --progn-rest)
  `(cond (,(nil? --progn-a) nil)
         (,(not (nil? --progn-rest)) ((lambda ()
                                        ,--progn-a
                                        (progn ,@--progn-rest))))
         (t ((lambda ()
               ,--progn-a)))))

(defmacro let* (--let*-bindings . body)
  `(if (not ,(nil? --let*-bindings))
       ((lambda (,(car (car --let*-bindings)))
          (let* ,(cdr --let*-bindings) ,@body))
        ,(cadr (car --let*-bindings)))
       (progn
         ,@body)))

(defun repeat (datum times)
  (if (eq times 0)
      nil
      (cons datum (repeat datum (- times 1)))))

(defmacro letrec (--letrec-bindings . body)
  `((lambda ,(primitive-map car --letrec-bindings)
      ,@(primitive-map (lambda (let-form)
                         `(setq ,(car let-form) ,(cadr let-form)))
                       --letrec-bindings)
      ,@body)
    ,@(repeat nil (length --letrec-bindings))))

(defmacro cond (--cond-binding . rest)
  `(if (not ,(nil? --cond-binding))
      (if ,(car --cond-binding)
          ,(cadr --cond-binding)
          (if (not ,(nil? rest))
              (cond ,@rest)))))

;;; WE CAN NOW USE WHEN, UNLESS, LETs, and COND

(defun flatten (seq)
  (cond ((nil? seq) nil)
        ((atom? (car seq)) (cons (car seq) (flatten (cdr seq))))
        (t (append (flatten (car seq)) (flatten (cdr seq))))))

(defmacro or (p . others)
  `(if ,p
       ,p
       (if ,(not (nil? others))
           (or ,@others))))

(defmacro and (p . others)
  `(if ,p
       (if ,(not (nil? others))
           (and ,@others)
           ,p)))

;;; WE CAN NOW USE FLATTEN, OR, and AND

(defun <= (n1 . numbers)
  (if (nil? numbers)
      t
      (let ((first-number (car numbers)))
        (when (or (eq first-number n1)
                (< n1 first-number))
            (apply <= numbers)))))

(defun >= (n1 . numbers)
  (if (nil? numbers)
      t
      (let ((first-number (car numbers)))
        (when (or (eq first-number n1)
                (> n1 first-number))
            (apply >= numbers)))))

;;; HIGH-LEVEL FUNCTIONS

(defun = (a b)
  (cond ((and (atom? a) (atom? b)) (eq a b))
        ((or (atom? a) (atom? b)) nil)
        (t (list= a b))))

(defun list= (list-a list-b)
  (cond ((and (nil? list-a) (nil? list-b)) t)
        ((or (nil? list-a) (nil? list-b)) nil)
        ((not (= (car list-a) (car list-b))) nil)
        (t (list= (cdr list-a) (cdr list-b)))))

(defun find (needle plist)
  (when plist
    (if (= (car (car plist)) needle)
        (cdr (car plist))
        (find needle (cdr plist)))))

(defun assoc (key value plist)
  (cons (cons key value) plist))

(defun memoize (f)
  (let ((function-table '()))
    (lambda (n . rest)
      (let* ((arg (cons n rest))
             (value (find arg function-table)))
        (if value
            value
            (let ((function-application-result (apply f arg)))
              (setq function-table (assoc arg function-application-result function-table))
              function-application-result))))))

(defun partial (f . initial-args)
  (lambda (n . rest)
    (apply f  (append initial-args
                      (cons n rest)))))
