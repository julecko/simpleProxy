while true; do
  curl -s -o /dev/null -w "%{http_code}\n" -x http://localhost:3128 --proxy-user User:Pass123 https://example.com &
  sleep 0.1
done
