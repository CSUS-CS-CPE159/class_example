size_t* setup_pagetable();
void kpagemake();
void page_fault_handler(size_t);
void enable_page_table(size_t *);