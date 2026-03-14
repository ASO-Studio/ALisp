(function add (a b)
  (return (+ a b))
)

(if (> (add 1 2) 5)
  (print "Y")
  (print "Y2")
elseif (> (add 5 5) 15)
  (print "EI")
  (print "EI2")
else
  (print "N")
)

(function test (x) (return x))
