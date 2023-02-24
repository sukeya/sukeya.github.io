---
layout: default
title: {{ site.title }}
---
ご意見、ご感想はGitHubのIssueまたはTwitterにご連絡ください。
私については[こちら](#about)をご覧ください。

# 投稿一覧
{% assign posts_group_by_year = site.posts | group_by_exp:"post", "post.date | date: '%Y'"  %}
{% for year in posts_group_by_year %}
## {{ year.items.first.date | date: "%Y" }}年
  {% for post in year.items %}
    {% assign posts_group_by_month = year.items | group_by_exp:"post", "post.date | date: '%m'"  %}
    {% for month in posts_group_by_month %}
### {{ month.items.first.date | date: "%m" }}月
        {% for post in month.items %}
- {{ post.date | date: "%e" }}日 [{{ post.title }}]({{ post.url }})
        {% endfor %}
    {% endfor %}
  {% endfor %}
{% endfor %}

# 私について {#about}
- GitHub [すけや](https://github.com/sukeya)
- Twitter [@sukeya](https://twitter.com/ReZeroRemLover)