---
layout: default
title: {{ site.title }}
---
ご意見、ご感想はGitHubのIssueまたはTwitterにご連絡ください。
私については[こちら](#about)をご覧ください。

# 投稿一覧
{{ site.posts | size }}
{% assign posts_group_by_year = site.posts | group_by_exp:"date", "date | date: '%Y'"  %}
{{ posts_group_by_year | size }}
{% for posts_at_year in posts_group_by_year %}
{{ posts_at_year | size }}
## {{ posts_at_year.items.first.date | date: "%Y" }}年
  {% assign posts_group_by_month = posts_at_year.items | group_by_exp:"date", "date | date: '%m'"  %}
  {% for posts_at_month in posts_group_by_month %}
    {{ posts_group_by_month | size }}
### {{ posts_at_month.items.first.date | date: "%m" }}月
    {% for post_at_month in posts_at_month.items %}
      {{ post_at_month | size }}
- {{ post_at_month.date | date: "%e" }}日 [{{ post_at_month.title }}]({{ post.url }})
    {% endfor %}
  {% endfor %}
{% endfor %}

# 私について {#about}
- GitHub [すけや](https://github.com/sukeya)
- Twitter [@sukeya](https://twitter.com/ReZeroRemLover)

[トップに戻る](#)
