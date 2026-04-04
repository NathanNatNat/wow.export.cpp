const log = require('../log');

const help_articles = [];

const filter_articles = (search) => {
	if (!search || search.trim() === '')
		return help_articles;

	const keywords = search.toLowerCase().trim().split(/\s+/);
	const scored = help_articles.map(article => {
		const has_default = article.tags.includes('default');
		let score = 0;

		for (const kw of keywords) {
			if (article.kb_id && article.kb_id.toLowerCase() === kw)
				score += 3;
			else if (article.kb_id && article.kb_id.toLowerCase().includes(kw))
				score += 2;

			for (const tag of article.tags) {
				if (tag === kw)
					score += 2;
				else if (tag.includes(kw))
					score += 1;
			}
		}

		return { article, matched: score, has_default };
	}).filter(s => s.matched > 0 || s.has_default);

	scored.sort((a, b) => b.matched - a.matched);
	return scored.map(s => s.article);
};

let filter_timeout = null;

module.exports = {
	register() {
		this.registerContextMenuOption('Help', 'help.svg');
	},

	template: `
		<div id="help-screen">
			<div class="help-list-container">
				<h1>Help</h1>
				<div class="filter">
					<input type="text" v-model="search_query" placeholder="Search help articles..."/>
				</div>
				<div id="help-articles">
					<div v-for="article in filtered_articles" @click="selected_article = article" class="help-article-item" :class="{ selected: selected_article === article }">
						<div class="help-article-title">{{ article.title }}</div>
						<div class="help-article-tags">
							<span v-if="article.kb_id" class="help-kb-id">{{ article.kb_id }}</span>
							<span>{{ article.tags.join(', ') }}</span>
						</div>
					</div>
				</div>
			</div>
			<div class="help-article-container">
				<component :is="$components.MarkdownContent" v-if="selected_article" :content="selected_article.body"></component>
				<div v-else class="help-placeholder">Select an article to view</div>
			</div>
			<input type="button" value="Go Back" @click="go_back"/>
		</div>
	`,

	data() {
		return {
			search_query: '',
			filtered_articles: [],
			selected_article: null
		};
	},

	methods: {
		go_back() {
			this.$modules.go_to_landing();
		},

		debounced_filter(search) {
			clearTimeout(filter_timeout);
			filter_timeout = setTimeout(() => {
				this.filtered_articles = filter_articles(search);
			}, 300);
		}
	},

	watch: {
		search_query(value) {
			this.debounced_filter(value);
		}
	},

	async mounted() {
		this.filtered_articles = help_articles;
	}
};
